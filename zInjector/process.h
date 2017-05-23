/* MIT License

Copyright(c) 2017 (https://github.com/EternityX/WindowsProcessManipulation)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE. */

#pragma once

#include "includes.h"

class Process
{
public:
	DWORD pid; // Process ID
	HANDLE handle; // Handle to process

	/**
	* Saves the process_id argument for later use.
	*
	* @param process_id				The Process ID(PID).
	*/
	explicit Process( DWORD process_id );

	// Calls CloseOpenHandle
	~Process( );

	/**
	* Wrapper function for OpenProcess
	*
	* @param desired_access			The access to the process object. This access right is checked against the security descriptor for the process.
	* @param inherit_handle			If this value is TRUE, processes created by this process will inherit the handle.
	*/
	bool Open( DWORD desired_access, BOOL inherit_handle );

	/**
	* Wrapper function for CloseHandle
	*/
	bool CloseOpenHandle( );

	/**
	* Terminates the process
	*
	* @param exit_code				The exit code to be used by the process and threads terminated as a result of this call.
	*/
	bool Terminate( UINT exit_code = EXIT_SUCCESS );

	/**
	* returns a handle for every thread within the process.
	*/
	std::vector<HANDLE> FetchThreads( );

	/**
	* Resumes the process by enumerating all threads and calling ResumeThread.
	*/
	bool Resume( );

	/**
	* EXPERIMENTAL: Resumes the process via the undocumented NtResumeProcess function.
	*/
	bool NtResume( );

	/**
	* Suspends the process by enumerating all threads and calling SuspendThread.
	*/
	bool Suspend( );

	/**
	* EXPERIMENTAL: Suspends the process via the undocumented NtSuspendProcess function.
	*/
	bool NtSuspend( );

	/**
	* Retrieves PIMAGE_NT_HEADERS structure
	*/
	PIMAGE_NT_HEADERS FetchImageHeader( );

	/**
	* Determines if the process is 64bit architecture by checking the file header for IMAGE_FILE_MACHINE_AMD64.
	*/
	bool Is64Bit( );

protected:
	using RtlAdjustPrivilege = NTSTATUS( WINAPI* )( ULONG, BOOLEAN, BOOLEAN, PBOOLEAN );

private:
	/**
	* Creates a map view for use with FetchDOSHeader.
	*/
	HANDLE CreateMapView( );

	/**
	* Retrieves the DOS header for use with FetchImageHeader.
	* @param map_view				Handle to the map view.
	*/
	static PIMAGE_DOS_HEADER FetchDOSHeader( HANDLE map_view );

	using NtSuspendProcess = NTSTATUS( WINAPI* )( HANDLE );
	using NtResumeProcess = NTSTATUS( WINAPI* )( HANDLE );
};

class Privileges : Process
{
	explicit Privileges( DWORD process_id )
		: Process( process_id )
	{
	}

	/**
	* EXPERIMENTAL: Enables or disables a privilege from the calling thread or process.
	*
	* @param privilege				Privilege index to change.
	* @param enable					If TRUE, then enable the privilege otherwise disable.
	* @param current_thread			If TRUE, then enable in calling thread, otherwise process.
	* @param enabled				Whether privilege was previously enabled or disabled.
	*/
	static bool RtlAdjustPrivileges( ULONG privilege, BOOLEAN enable, BOOLEAN current_thread, PBOOLEAN enabled );

protected:
	HANDLE FetchTokenHandle( DWORD desired_access );
};
