#include "utilities.h"

void RaiseError( const char *fmt, ... )
{
	char buffer[ 1024 ];

	va_list args;

	va_start( args, fmt );
	vsprintf_s( buffer, sizeof( buffer ), fmt, args );
	va_end( args );

	MessageBox( nullptr, buffer, "zInjector", MB_OK | MB_ICONASTERISK );

	ExitProcess( EXIT_FAILURE );
}

/// <summary>
/// Grab process by Name (e.g. notepad.exe)
/// </summary>
/// <param name="process_name">The process name</param>
/// <returns>Process ID (PID)</returns>
uint32_t GrabProcessByName( char *process_name )
{
	HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	uint32_t count = 0;
	uint32_t pid = 0;

	if ( snap == INVALID_HANDLE_VALUE )
	{
		throw GetLastError( );
	}

	// check if process is running
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

	CloseHandle( snap );

	return pid;
}

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns></returns>
bool CreateRemoteThreadMethod( uint32_t pid, const char *dll_path )
{
	HANDLE process;
	process = OpenProcess( PROCESS_ALL_ACCESS, false, pid );

	LPVOID loadLibraryAddress;
	loadLibraryAddress = LPVOID( GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" ) );

	LPVOID memory;
	memory = LPVOID( VirtualAllocEx( process, nullptr, strlen( dll_path ) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ) );

	WriteProcessMemory( process, LPVOID( memory ), dll_path, strlen( dll_path ) + 1, nullptr );
	CreateRemoteThread( process, nullptr, NULL, LPTHREAD_START_ROUTINE( loadLibraryAddress ), LPVOID( memory ), NULL, nullptr );

	CloseHandle( process );
	VirtualFreeEx( process, LPVOID( memory ), 0, MEM_RELEASE );

	return true;
}
