#include "process.h"
#include <iostream>
#include <string>

Process::Process( DWORD process_id, DWORD desired_access )
{
	m_pid = process_id;
	m_desired_access = desired_access;
	m_hwnd = Process::FetchPrimaryWindowHandle( );
	m_handle = nullptr;
	m_file = nullptr;
}

Process::Process( std::string process_name, DWORD desired_access )
{
	m_pid = Process::FetchProcessByName( process_name );
	m_desired_access = desired_access;
	m_hwnd = Process::FetchPrimaryWindowHandle( );
	m_handle = nullptr;
	m_file = nullptr;
	m_name = process_name;
}

Process::~Process( )
{
	Process::CloseOpenHandle( );
}

bool Process::IsValid( ) const
{
	if ( m_handle == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	if ( !WaitForSingleObject( m_handle, 5000 ) )
	{
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

void Process::SetDesiredAccess( DWORD desired_access )
{
	m_desired_access = desired_access;
}

void Process::SetModuleName( std::string module_name )
{
	m_name = module_name;
}

void Process::SetPrimaryWindowHandle( HWND window_handle )
{
	m_handle = window_handle;
}

DWORD Process::GetPid( ) const
{
	return m_pid;
}

DWORD Process::GetDesiredAccess( ) const
{
	return m_desired_access;
}

HWND Process::GetPrimaryWindowHandle( ) const
{
	return m_hwnd;
}

HANDLE Process::GetHandle( ) const
{
	return m_handle;
}

HANDLE Process::GetFile( ) const
{
	return m_file;
}

std::string Process::GetModuleName( ) const
{
	return m_name;
}

bool Process::Open( BOOL inherit_handle )
{
	m_handle = OpenProcess( m_desired_access, inherit_handle, m_pid );
	if ( m_handle == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	return true;
}

bool Process::CloseOpenHandle( ) const
{
	if ( m_handle == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	if ( !CloseHandle( m_handle ) )
	{
		return false;
	}

	return true;
}

DWORD Process::FetchProcessByName( std::string process_name )
{
	int count = 0;
	int pid = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( snapshot == INVALID_HANDLE_VALUE )
	{
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

	if ( count > 1 )
	{
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

	if ( !result && pair.first )
	{
		return pair.first;
	}

	return nullptr;
}

std::string Process::FetchProcessImageFileName( ) const
{
	char process_image_name[ MAX_PATH ];

	if ( !K32GetProcessImageFileNameA( m_handle, process_image_name, MAX_PATH ) )
	{
		return { };
	}

	return { process_image_name };
}

std::string Process::FetchModuleFileName( ) const
{
	char module_name[ MAX_PATH ];

	if ( !K32GetModuleFileNameExA( m_handle, nullptr, module_name, MAX_PATH ) )
	{
		return { };
	}

	return { module_name };
}

bool Process::Terminate( UINT exit_code ) const
{
	if ( !TerminateProcess( m_handle, exit_code ) )
	{
		return false;
	}

	return true;
}

std::vector<HANDLE> Process::FetchThreads( ) const
{
	std::vector<HANDLE> thread_handles;

	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
	if ( snapshot == INVALID_HANDLE_VALUE )
	{
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

	if ( !CloseHandle( snapshot ) )
	{
		return { };
	}

	return thread_handles;
}

bool Process::Resume( ) const
{
	auto thread_handles = FetchThreads( );
	for ( auto &thread : thread_handles )
	{
		if ( !ResumeThread( thread ) )
		{
			return false;
		}
	}

	return true;
}

bool Process::NtResume( ) const
{
	static auto ResumeProcess = reinterpret_cast<Process::NtResumeProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtResumeProcess" ) );
	if ( NT_ERROR( ResumeProcess( Process::m_handle ) ) )
	{
		return false;
	}

	return true;
}

bool Process::Suspend( ) const
{
	auto thread_handles = FetchThreads( );
	for ( auto &thread : thread_handles )
	{
		if ( !SuspendThread( thread ) )
		{
			return false;
		}
	}

	return true;
}

bool Process::NtSuspend( ) const
{
	static auto SuspendProcess = reinterpret_cast<Process::NtSuspendProcess>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtSuspendProcess" ) );
	if ( NT_ERROR( SuspendProcess( m_handle ) ) )
	{
		return false;
	}

	return true;
}

HANDLE Process::CreateMapView( )
{
	TCHAR buffer[ MAX_PATH ];
	if ( !K32GetModuleFileNameExA( m_handle, nullptr, buffer, MAX_PATH ) )
	{
		return nullptr;
	}

	m_file = CreateFileA( buffer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( m_file == INVALID_HANDLE_VALUE )
	{
		return nullptr;
	}

	HANDLE map = CreateFileMappingA( m_file, nullptr, PAGE_READONLY, 0, 0, nullptr );
	if ( !map )
	{
		return nullptr;
	}

	HANDLE map_view = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
	if ( !map_view )
	{
		return nullptr;
	}

	if ( !CloseHandle( m_file ) && !CloseHandle( map ) && !CloseHandle( map_view ) )
	{
		return nullptr;
	}

	return map_view;
}

PIMAGE_DOS_HEADER Process::FetchDOSHeader( HANDLE map_view )
{
	PIMAGE_DOS_HEADER dos_header = static_cast<PIMAGE_DOS_HEADER>( map_view );
	if ( dos_header->e_magic != IMAGE_DOS_SIGNATURE )
	{
		return nullptr;
	}

	return dos_header;
}

PIMAGE_NT_HEADERS Process::FetchImageHeader( )
{
	PIMAGE_DOS_HEADER dos_header = Process::FetchDOSHeader( Process::CreateMapView( ) );
	if ( !dos_header )
	{
		return nullptr;
	}

	PIMAGE_NT_HEADERS image_header = reinterpret_cast<PIMAGE_NT_HEADERS>( reinterpret_cast<PCHAR>( dos_header ) + dos_header->e_lfanew );
	if ( image_header->Signature != IMAGE_NT_SIGNATURE )
	{
		return nullptr;
	}

	return image_header;
}

bool Process::Is64Bit( )
{
	PIMAGE_NT_HEADERS nt_header = Process::FetchImageHeader( );
	if ( !nt_header )
	{
		return false;
	}

	if ( nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 )
	{
		return true;
	}

	return false;
}

// basic implementation
std::vector<Process::IMPORT_INFO> Process::FetchImports( )
{
	std::vector<IMPORT_INFO> return_imports;

	HANDLE file = CreateFileA( FetchModuleFileName( ).c_str( ), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( file == INVALID_HANDLE_VALUE )
	{
		CloseHandle( file );
		return { };
	}
	CloseHandle( file );

	PIMAGE_NT_HEADERS nt_header = Process::FetchImageHeader( );
	if ( !nt_header )
	{
		return { };
	}

	auto section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>( reinterpret_cast<DWORD>( nt_header ) + sizeof( IMAGE_NT_HEADERS ) );

	DWORD section = 0;

	DWORD rva = nt_header->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress;
	if ( !rva )
	{
		return { };
	}

	do
	{
		section_header++;
		section++;
	} while ( section < nt_header->FileHeader.NumberOfSections && section_header->VirtualAddress <= rva );

	section_header--;

	DWORD base = reinterpret_cast<DWORD>( Process::CreateMapView( ) ) + section_header->PointerToRawData;
	auto import_descriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>( base + ( rva - section_header->VirtualAddress ) );

	IMPORT_INFO import_info;
	while ( import_descriptor->Name != 0 )
	{
		import_info.library = base + ( import_descriptor->Name - section_header->VirtualAddress );

		auto thunk_data = reinterpret_cast<PIMAGE_THUNK_DATA>( base + ( import_descriptor->FirstThunk - section_header->VirtualAddress ) );
		while ( thunk_data->u1.AddressOfData != 0 )
		{
			// need to get ordinals too
			if ( !( thunk_data->u1.AddressOfData & IMAGE_ORDINAL_FLAG ) )
			{
				import_info.function = base + ( thunk_data->u1.AddressOfData - section_header->VirtualAddress + 2 );
			}

			return_imports.push_back( import_info );

			thunk_data++;
		}

		import_descriptor++;
	}

	return return_imports;
}

std::vector<Process::HANDLE_INFO> Process::FetchHandles( ) const
{
	std::vector<HANDLE_INFO> return_handles;

	static auto QuerySystemInformation = reinterpret_cast<Process::NtQuerySystemInformation>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQuerySystemInformation" ) );

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
	}

	auto system_handle_info = reinterpret_cast<PSYSTEM_HANDLE_INFORMATION_EX>( buffer.get( ) );
	for ( auto i = 0; i < system_handle_info->number_of_handles; i++ )
	{
		HANDLE process_copy = nullptr;

		auto system_handle = &system_handle_info->handles[ i ];

		if ( system_handle->object_type_index == 7 )
		{
			HANDLE process_handle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, system_handle->unique_pid );
			if ( DuplicateHandle( process_handle, reinterpret_cast<HANDLE>( system_handle->handle_value ), GetCurrentProcess( ), &process_copy, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, 0 ) )
			{
				HANDLE_INFO handle_info = { 0, nullptr };

				if ( GetProcessId( process_copy ) == m_pid )
				{
					handle_info.pid = system_handle->unique_pid;
					handle_info.process = reinterpret_cast<HANDLE>( system_handle->handle_value );
					return_handles.push_back( handle_info );
				}
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

	if ( !OpenProcessToken( m_handle, desired_access, &token_handle ) )
	{
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
	if ( NT_ERROR( AdjustPrivileges( privilege, enable, current_thread, enabled ) ) )
	{
		return false;
	}

	return true;
}