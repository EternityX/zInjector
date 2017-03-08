#include "createremotethread.h"

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns>True on success</returns>
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