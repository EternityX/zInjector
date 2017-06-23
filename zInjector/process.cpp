#include "process.h"

namespace wpm {
	Process::Process( DWORD process_id, DWORD desired_access )
	{
		m_pid = process_id;
		m_desired_access = desired_access;
		m_hwnd = Process::FetchPrimaryWindowHandle( );
		m_handle = nullptr;
	}

	Process::Process( std::string process_name, DWORD desired_access )
	{
		m_pid = Process::FetchProcessByName( process_name );
		m_desired_access = desired_access;
		m_hwnd = Process::FetchPrimaryWindowHandle( );
		m_handle = nullptr;
		m_name = process_name;
	}

	Process::~Process( )
	{
		Process::CloseOpenHandle( );
	}

	bool Process::IsValid( ) const
	{
		if ( m_handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		return true;
	}

	BOOL Process::IsNotResponding( ) const
	{
		return IsHungAppWindow( m_hwnd );
	}

	LRESULT Process::IsNotResponding( UINT timeout ) const
	{
		if ( !SendMessageTimeoutA( m_hwnd, WM_NULL, 0, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG, timeout, nullptr ) )
			return false;

		return true;
	}

	bool Process::Open( BOOL inherit_handle )
	{
		m_handle = OpenProcess( m_desired_access, inherit_handle, m_pid );
		if ( m_handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		return true;
	}

	bool Process::CloseOpenHandle( ) const
	{
		if ( m_handle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		if ( !CloseHandle( m_handle ) ) {
			return false;
		}

		return true;
	}

	DWORD Process::FetchProcessByName( std::string process_name )
	{
		int count = 0;
		int pid = 0;

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

		if ( snapshot == INVALID_HANDLE_VALUE ) {
			return -1;
		}

		PROCESSENTRY32 process_entry = { sizeof( PROCESSENTRY32 ) };
		Process32Next( snapshot, &process_entry );
		do
		{
			if ( !process_name.compare( std::string( process_entry.szExeFile ) ) )
			{
				count++;
				pid = process_entry.th32ProcessID;
			}
		} while ( Process32Next( snapshot, &process_entry ) );

		if ( count > 1 ) {
			pid = -1;
		}

		CloseHandle( snapshot );

		return pid;
	}

	HWND Process::FetchPrimaryWindowHandle( )
	{
		std::pair<HWND, DWORD> pair = { nullptr, m_pid };

		BOOL result = EnumWindows( [ ]( HWND hwnd, LPARAM lparam ) -> BOOL
		{
			auto params = reinterpret_cast< std::pair<HWND, DWORD>* >( lparam );

			DWORD processId;
			if ( GetWindowThreadProcessId( hwnd, &processId ) && processId == params->second && GetWindow( hwnd, GW_OWNER ) == nullptr )
			{
				params->first = hwnd;
				return FALSE;
			}

			return TRUE;
		}, reinterpret_cast<LPARAM>( &pair ) );

		if ( !result && pair.first ) {
			return pair.first;
		}

		return nullptr;
	}

	std::string Process::FetchProcessImageFileName( ) const
	{
		char process_image_name[ MAX_PATH ];

		if ( !K32GetProcessImageFileNameA( m_handle, process_image_name, MAX_PATH ) ) {
			return { };
		}

		return { process_image_name };
	}

	std::string Process::FetchModuleFileName( ) const
	{
		char module_name[ MAX_PATH ];

		if ( !K32GetModuleFileNameExA( m_handle, nullptr, module_name, MAX_PATH ) ) {
			return { };
		}

		return { module_name };
	}

	bool Process::Terminate( UINT exit_code ) const
	{
		if ( !TerminateProcess( m_handle, exit_code ) ) {
			return false;
		}

		return true;
	}

	std::vector<HANDLE> Process::FetchThreads( ) const
	{
		std::vector<HANDLE> thread_handles;

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if ( snapshot == INVALID_HANDLE_VALUE ) {
			return { };
		}

		THREADENTRY32 thread_entry = { sizeof( THREADENTRY32 ) };
		Thread32First( snapshot, &thread_entry );
		do
		{
			if ( thread_entry.th32OwnerProcessID == m_pid )
			{
				HANDLE thread_handle = OpenThread( THREAD_QUERY_INFORMATION, FALSE, thread_entry.th32ThreadID );
				thread_handles.push_back( thread_handle );
			}
		} while ( Thread32Next( snapshot, &thread_entry ) );

		if ( !CloseHandle( snapshot ) ) {
			return { };
		}

		return thread_handles;
	}

	bool Process::Resume( ) const
	{
		auto thread_handles = FetchThreads( );
		for ( auto &thread : thread_handles ) {
			if ( !ResumeThread( thread ) ) {
				return false;
			}
		}

		return true;
	}

	bool Process::NtResume( ) const
	{
		static auto ResumeProcess = reinterpret_cast<Process::NtResumeProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtResumeProcess" ) );
		if ( NT_ERROR( ResumeProcess( m_handle ) ) ) {
			return false;
		}

		return true;
	}

	bool Process::Suspend( ) const
	{
		auto thread_handles = FetchThreads( );
		for ( auto &thread : thread_handles ) {
			if ( !SuspendThread( thread ) ) {
				return false;
			}
		}

		return true;
	}

	bool Process::NtSuspend( ) const
	{
		static auto SuspendProcess = reinterpret_cast<Process::NtSuspendProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtSuspendProcess" ) );
		if ( NT_ERROR( SuspendProcess( m_handle ) ) ) {
			return false;
		}

		return true;
	}

	std::vector<Process::HANDLE_INFO> Process::FetchHandles( ) const
	{
		std::vector<HANDLE_INFO> return_handles;

		static auto QuerySystemInformation = reinterpret_cast<Process::NtQuerySystemInformation>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQuerySystemInformation" ) );
		static auto QueryObject = reinterpret_cast<Process::NtQueryObject>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQueryObject" ) );

		ULONG return_length = 0;
		ULONG buffer_size = 1 << 20; // 1048576
		std::unique_ptr<BYTE[ ]> buffer( new BYTE[ buffer_size ] );

		NTSTATUS status;

		do
		{
			status = QuerySystemInformation( SystemExtendedHandleInformation, buffer.get( ), buffer_size, &buffer_size );
			if ( status == STATUS_INFO_LENGTH_MISMATCH )
			{
				buffer_size = ( return_length > buffer_size ) ? return_length : ( buffer_size * 2 );
				buffer.reset( new BYTE[ buffer_size ] );
			}
		} while ( status == STATUS_INFO_LENGTH_MISMATCH && buffer_size < 1 << 24 ); // 16777216

		if ( NT_ERROR( status ) )
		{
			buffer.reset( );
			return { };
		}

		auto system_handle_info = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>( buffer.get( ) );
		for ( DWORD i = 0; i < system_handle_info->number_of_handles; i++ )
		{
			HANDLE process_copy = nullptr;

			auto system_handle = &system_handle_info->handles[ i ];

			HANDLE process_handle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, system_handle->unique_pid );
			if ( DuplicateHandle( process_handle, reinterpret_cast<HANDLE>( system_handle->handle_value ), GetCurrentProcess( ), &process_copy, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, 0 ) )
			{
				if ( GetLastError( ) == ERROR_NOT_SUPPORTED ) {
					CloseHandle( process_handle );
					break;
				}

				HANDLE_INFO handle_info = { 0, nullptr };
				if ( GetProcessId( process_copy ) == m_pid )
				{
					handle_info.pid = system_handle->unique_pid;
					handle_info.process = reinterpret_cast<HANDLE>( system_handle->handle_value );
					return_handles.push_back( handle_info );
				}

				CloseHandle( process_handle );
			}

			CloseHandle( process_copy );
		}

		return return_handles;
	}

	HANDLE Process::FetchAccessToken( DWORD desired_access ) const
	{
		HANDLE token_handle;

		if ( !OpenProcessToken( m_handle, desired_access, &token_handle ) ) {
			return nullptr;
		}

		return token_handle;
	}

	bool Process::SetPrivilege( LPCTSTR name, BOOL enable_privilege ) const
	{
		TOKEN_PRIVILEGES privilege = { 0, 0, 0, 0 };
		LUID luid = { 0, 0 };

		HANDLE token = Process::FetchAccessToken( TOKEN_ADJUST_PRIVILEGES );

		if ( !LookupPrivilegeValueA( nullptr, name, &luid ) ) 
		{
			CloseHandle( token );
			return false;
		}

		privilege.PrivilegeCount = 1;
		privilege.Privileges[ 0 ].Luid = luid;
		privilege.Privileges[ 0 ].Attributes = enable_privilege ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;

		if ( !AdjustTokenPrivileges( token, FALSE, &privilege, 0, nullptr, nullptr ) ) 
		{
			CloseHandle( token );
			return false;
		}

		CloseHandle( token );

		return true;
	}

	bool Process::RtlAdjustPrivileges( ULONG privilege, BOOLEAN enable, BOOLEAN current_thread, PBOOLEAN enabled )
	{
		static auto AdjustPrivileges = reinterpret_cast<Process::RtlAdjustPrivilege>( GetProcAddress( GetModuleHandleA( "ntdll" ), "RtlAdjustPrivilege" ) );
		if ( NT_ERROR( AdjustPrivileges( privilege, enable, current_thread, enabled ) ) ) {
			return false;
		}

		return true;
	}

	bool Process::LoadLibraryExternal( std::string library_path ) const
	{
		LPVOID base_address = VirtualAllocEx( m_handle, nullptr, library_path.size( ) + 1, MEM_COMMIT, PAGE_READWRITE );

		if ( !WriteProcessMemory( m_handle, base_address, library_path.c_str( ), library_path.size( ) + 1, nullptr ) )
		{
			VirtualFreeEx( m_handle, base_address, 0, MEM_RELEASE );
			return false;
		}

		HANDLE thread = CreateRemoteThread( m_handle, nullptr, 0, LPTHREAD_START_ROUTINE( GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" ) ), base_address, 0, nullptr );
		if ( thread && thread != INVALID_HANDLE_VALUE )
		{
			WaitForSingleObject( thread, 5000 );
			CloseHandle( thread );
		}
		else
		{
			VirtualFreeEx( m_handle, base_address, 0, MEM_RELEASE );
			return false;
		}

		return true;
	}
}
