#include "process.h"

namespace wpm {
	Process::Process( DWORD process_id, DWORD desired_access ) {
		m_pid = process_id;
		m_desired_access = desired_access;
		m_hwnd = Process::fetch_primary_window_handle( );
		m_handle = nullptr;
	}

	Process::Process( const std::string &process_name, DWORD desired_access ) {
		m_pid = Process::fetch_process_by_name( process_name );
		m_desired_access = desired_access;
		m_hwnd = Process::fetch_primary_window_handle();
		m_handle = nullptr;
		m_name = process_name;
	}

	Process::~Process() {
		Process::close_open_handle();
	}

	bool Process::is_valid() const {
		return m_handle != INVALID_HANDLE_VALUE;
	}

	BOOL Process::is_not_responding( ) const {
		return IsHungAppWindow( m_hwnd );
	}

	LRESULT Process::is_not_responding( UINT timeout ) const {
		if( !SendMessageTimeoutA( m_hwnd, WM_NULL, 0, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG, timeout, nullptr ) )
			return false;

		return true;
	}

	bool Process::open( BOOL inherit_handle ) {
		m_handle = OpenProcess( m_desired_access, inherit_handle, m_pid );
		return m_handle != INVALID_HANDLE_VALUE;
	}

	bool Process::close_open_handle() const {
		return !( m_handle == INVALID_HANDLE_VALUE || !CloseHandle( m_handle ) );
	}

	DWORD Process::fetch_process_by_name( const std::string &process_name ) {
		int count = 0;
		int pid = 0;

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

		if( snapshot == INVALID_HANDLE_VALUE )
			return -1;

		PROCESSENTRY32 process_entry = { sizeof( PROCESSENTRY32 ) };
		Process32Next( snapshot, &process_entry );
		do {
			if( process_name != std::string( process_entry.szExeFile ) ) {
				count++;
				pid = process_entry.th32ProcessID;
			}
		} while ( Process32Next( snapshot, &process_entry ) );

		if( count > 1 )
			pid = -1;

		CloseHandle( snapshot );

		return pid;
	}

	HWND Process::fetch_primary_window_handle() {
		std::pair<HWND, DWORD> pair = { nullptr, m_pid };

		auto result = EnumWindows( []( HWND hwnd, LPARAM lparam ) {
			auto params = reinterpret_cast<std::pair<HWND, DWORD>*>( lparam );

			DWORD processId;
			if( GetWindowThreadProcessId( hwnd, &processId ) && processId == params->second && GetWindow( hwnd, GW_OWNER ) == nullptr ) {
				params->first = hwnd;
				return FALSE;
			}

			return TRUE;
		}, reinterpret_cast<LPARAM>( &pair ) );

		if( !result && pair.first )
			return pair.first;

		return nullptr;
	}

	std::string Process::fetch_process_image_file_name() const {
		char process_image_name[ MAX_PATH ];

		if( !K32GetProcessImageFileNameA( m_handle, process_image_name, MAX_PATH ) )
			return {};

		return { process_image_name };
	}

	std::string Process::fetch_module_file_name() const {
		char module_name[ MAX_PATH ];

		if( !K32GetModuleFileNameExA( m_handle, nullptr, module_name, MAX_PATH ) )
			return {};

		return { module_name };
	}

	bool Process::terminate( UINT exit_code ) const {
		return TerminateProcess( m_handle, exit_code ) != 0;
	}

	std::vector<HANDLE> Process::fetch_threads() const {
		std::vector<HANDLE> thread_handles;

		HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
		if( snapshot == INVALID_HANDLE_VALUE )
			return {};

		THREADENTRY32 thread_entry = { sizeof( THREADENTRY32 ) };
		Thread32First( snapshot, &thread_entry );
		do {
			if( thread_entry.th32OwnerProcessID == m_pid ) {
				HANDLE thread_handle = OpenThread( THREAD_QUERY_INFORMATION, FALSE, thread_entry.th32ThreadID );
				thread_handles.push_back( thread_handle );
			}
		} while( Thread32Next( snapshot, &thread_entry ) );

		if( !CloseHandle( snapshot ) )
			return {};

		return thread_handles;
	}

	bool Process::resume( ) const {
		auto thread_handles = fetch_threads();
		for( auto &thread : thread_handles ) {
			if( !ResumeThread( thread ) )
				return false;
		}

		return true;
	}

	bool Process::nt_resume() const {
		static auto resume_process = reinterpret_cast<Process::NtResumeProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtResumeProcess" ) );
		if( NT_ERROR( resume_process( m_handle ) ) )
			return false;

		return true;
	}

	bool Process::suspend() const {
		auto thread_handles = fetch_threads();
		for( auto &thread : thread_handles ) {
			if( !SuspendThread( thread ) )
				return false;
		}

		return true;
	}

	bool Process::nt_suspend() const {
		static auto suspend_process = reinterpret_cast<Process::NtSuspendProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtSuspendProcess" ) );
		if( NT_ERROR( suspend_process( m_handle ) ) )
			return false;

		return true;
	}

	std::vector<Process::HANDLE_INFO> Process::fetch_handles() const {
		std::vector<HANDLE_INFO> return_handles;

		static auto query_system_information = reinterpret_cast<Process::NtQuerySystemInformation>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQuerySystemInformation" ) );
		static auto query_object = reinterpret_cast<Process::NtQueryObject>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQueryObject" ) );

		ULONG return_length = 0;
		ULONG buffer_size = 1 << 20;
		std::unique_ptr<BYTE[]> buffer( new BYTE[ buffer_size ] );

		NTSTATUS status;
		do {
			status = query_system_information( SystemExtendedHandleInformation, buffer.get(), buffer_size, &buffer_size );
			if( status == STATUS_INFO_LENGTH_MISMATCH ) {
				buffer_size = return_length > buffer_size ? return_length : buffer_size * 2;
				buffer.reset( new BYTE[ buffer_size ] );
			}
		} while ( status == STATUS_INFO_LENGTH_MISMATCH && buffer_size < 1 << 24 );

		if( NT_ERROR( status ) ) {
			buffer.reset();
			return {};
		}

		auto system_handle_info = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>( buffer.get() );
		for( unsigned long i = 0; i < system_handle_info->number_of_handles; i++ ) {
			HANDLE process_copy = nullptr;

			auto system_handle = &system_handle_info->handles[ i ];

			HANDLE process_handle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, system_handle->unique_pid );
			if( DuplicateHandle( process_handle, reinterpret_cast<HANDLE>( system_handle->handle_value ), GetCurrentProcess(), &process_copy, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, 0 ) ) {
				if( GetLastError() == ERROR_NOT_SUPPORTED ) {
					CloseHandle( process_handle );
					break;
				}

				HANDLE_INFO handle_info = { 0, nullptr };
				if( GetProcessId( process_copy ) == m_pid ) {
					handle_info.m_pid = system_handle->unique_pid;
					handle_info.m_process = reinterpret_cast<HANDLE>( system_handle->handle_value );
					return_handles.push_back( handle_info );
				}

				CloseHandle( process_handle );
			}

			CloseHandle( process_copy );
		}

		return return_handles;
	}

	HANDLE Process::fetch_access_token( DWORD desired_access ) const {
		HANDLE token_handle;

		if( !OpenProcessToken( m_handle, desired_access, &token_handle ) )
			return nullptr;

		return token_handle;
	}

	bool Process::set_privilege( LPCTSTR name, BOOL enable_privilege ) const {
		TOKEN_PRIVILEGES privilege = { 0, 0, 0, 0 };
		LUID luid = { 0, 0 };

		HANDLE token = Process::fetch_access_token( TOKEN_ADJUST_PRIVILEGES );

		if( !LookupPrivilegeValueA( nullptr, name, &luid ) ) {
			CloseHandle( token );
			return false;
		}

		privilege.PrivilegeCount = 1;
		privilege.Privileges[ 0 ].Luid = luid;
		privilege.Privileges[ 0 ].Attributes = enable_privilege ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;

		if( !AdjustTokenPrivileges( token, FALSE, &privilege, 0, nullptr, nullptr ) )  {
			CloseHandle( token );
			return false;
		}

		CloseHandle( token );

		return true;
	}

	bool Process::rtl_adjust_privileges( ULONG privilege, BOOLEAN enable, BOOLEAN current_thread, PBOOLEAN enabled ) {
		static auto adjust_privileges = reinterpret_cast<Process::RtlAdjustPrivilege>( GetProcAddress( GetModuleHandleA( "ntdll" ), "RtlAdjustPrivilege" ) );
		if( NT_ERROR( adjust_privileges( privilege, enable, current_thread, enabled ) ) )
			return false;

		return true;
	}

	bool Process::load_library_external( const std::string &library_path ) const {
		LPVOID base_address = VirtualAllocEx( m_handle, nullptr, library_path.size() + 1, MEM_COMMIT, PAGE_READWRITE );

		if( !WriteProcessMemory( m_handle, base_address, library_path.c_str(), library_path.size() + 1, nullptr ) ) {
			VirtualFreeEx( m_handle, base_address, 0, MEM_RELEASE );
			return false;
		}

		HANDLE thread = CreateRemoteThread( m_handle, nullptr, 0, LPTHREAD_START_ROUTINE( GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" ) ), base_address, 0, nullptr );
		if( thread && thread != INVALID_HANDLE_VALUE ) {
			WaitForSingleObject( thread, 5000 );
			CloseHandle( thread );
		}
		else {
			VirtualFreeEx( m_handle, base_address, 0, MEM_RELEASE );
			return false;
		}

		return true;
	}
}
