#include "utilities.h"

void RaiseError( )
{
	LPSTR buffer = nullptr;

	// FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
	size_t size = FormatMessageA( 0x100 | 0x1000 | 0x200, nullptr, GetLastError( ), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast< LPSTR >( &buffer ), 0, nullptr );

	std::string text( buffer, size );
	std::cout << "ERROR: " << text << std::endl;
}

/// <summary>
/// Grab process by Name (e.g. notepad.exe)
/// </summary>
/// <param name="process_name">The process name</param>
/// <returns>Process ID (PID)</returns>
unsigned int GrabProcessByName( char *process_name )
{
	HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	unsigned int count = 0;
	unsigned int pid = 0;

	if ( snap == INVALID_HANDLE_VALUE )
	{
		RaiseError( );
	}

	// make sure process is running
	if ( !WaitForSingleObject( snap, 5000 ) )
	{
		return 0;
	}

	PROCESSENTRY32 proc;
	proc.dwSize = sizeof( PROCESSENTRY32 );
	bool ret = Process32Next( snap, &proc );

	while ( ret )
	{
		if ( !_stricmp( proc.szExeFile, process_name ) )
		{
			count++;
			pid = proc.th32ProcessID;
		}
		ret = Process32Next( snap, &proc );
	}

	if ( count > 1 )
	{
		pid = -1;
	}

	if ( !CloseHandle( snap ) )
		RaiseError( );

	return pid;
}

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns>Will return false on failure. Use RaiseError( ) to retrieve the error message.</returns>
bool CreateRemoteThreadMethod( unsigned int pid, const char *dll_path )
{
	HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, false, pid );
	if ( !process )
		return false;

	LPVOID memory = LPVOID( VirtualAllocEx( process, nullptr, strlen( dll_path ) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
	if ( !memory )
		return false;

	if ( !WriteProcessMemory( process, memory, dll_path, strlen( dll_path ) + 1, nullptr ) )
		return false;

	if ( !CreateRemoteThread( process, nullptr, NULL, LPTHREAD_START_ROUTINE( GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" ) ), memory, NULL, nullptr ) )
		return false;

	if ( !CloseHandle( process ) )
		return false;
	
	VirtualFreeEx( process, memory, 0, MEM_RELEASE );

	return true;
}