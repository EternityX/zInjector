#include "createremotethread.h"

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns>True on success</returns>
bool CreateRemoteThreadMethod( int pid, std::string dll_path )
{
	// Allocate memory for our module inside the target process
	LPVOID base_address = VirtualAllocEx( process->handle, nullptr, dll_path.size( ) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if ( !base_address )
		return false;

	// Grab the LoadLibraryA address
	LPVOID load_library_address = GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" );
	if ( !load_library_address )
		return false;

	// Copy the module name/path to the previously allocated memory (required by LoadLibraryA)
	if ( !WriteProcessMemory( process->handle, base_address, dll_path.c_str( ), dll_path.size( ) + 1, nullptr ) )
		return false;

	// Create a new thread in our target process pointing to our LoadLibraryA function
	if ( !CreateRemoteThread( process->handle, nullptr, NULL, LPTHREAD_START_ROUTINE( load_library_address ), base_address, NULL, nullptr ) )
		return false;

	// VirtualFreeEx( process->handle, base_address, NULL, MEM_RELEASE );

	return true;
}