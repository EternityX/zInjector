#include "createremotethread.h"

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns>True on success</returns>
bool CreateRemoteThreadMethod( int pid, std::string dll_path )
{
	// With all possible rights, grab the handle to the process
	HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, false, pid );
	if ( !process )
		return false;

	// Allocate memory for our module inside the target process
	LPVOID baseAddress = VirtualAllocEx( process, nullptr, dll_path.size( ) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if ( !baseAddress )
		return false;

	// Grab the LoadLibraryA address
	LPVOID loadLibraryAddress = GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" );
	if ( !loadLibraryAddress )
		return false;

	// Copy the module name/path to the previously allocated memory (required by LoadLibraryA)
	if ( !WriteProcessMemory( process, baseAddress, dll_path.c_str( ), dll_path.size( ) + 1, nullptr ) )
		return false;

	// Create a new thread in our target process pointing to our LoadLibraryA function
	// NOTE: NtCreateThreadEx and RtlCreateUserThread can also be used instead of CreateRemoteThread
	if ( !CreateRemoteThread( process, nullptr, NULL, LPTHREAD_START_ROUTINE( loadLibraryAddress ), baseAddress, NULL, nullptr ) )
		return false;

	if ( !CloseHandle( process ) )
		return false;

	// Make sure to free our allocated memory
	VirtualFreeEx( process, baseAddress, 0, MEM_RELEASE );

	return true;
}