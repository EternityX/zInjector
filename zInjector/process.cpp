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
