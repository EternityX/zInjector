/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <winternl.h>
#include <ntstatus.h>

#include <memory>

#include <vector>

#include <TlHelp32.h>

#include "Shlwapi.h"
#pragma comment(lib, "shlwapi")

#include <psapi.h>
#pragma comment(lib, "psapi")

namespace wpm
{
	class Process
	{
	public:
		struct HANDLE_INFO
		{
			DWORD pid;
			HANDLE process;
		};

	private:
		// SystemHandleInformation
		typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
		{
			USHORT unique_pid;
			USHORT creator_backtrace_index;
			UCHAR object_type_index;
			UCHAR handle_attributes;
			USHORT handle_value;
			PVOID object;
			ULONG granted_access;
		} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

		typedef struct _SYSTEM_HANDLE_INFORMATION
		{
			ULONG number_of_handles;
			SYSTEM_HANDLE_TABLE_ENTRY_INFO handles[ 1 ];
		} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

		// SystemExtendedHandleInformation
		typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
		{
			PVOID object;
			ULONG_PTR unique_pid;
			ULONG_PTR handle_value;
			ULONG granted_access;
			USHORT creator_backtrace_index;
			USHORT object_type_index;
			ULONG handle_attributes;
			ULONG reserved;
		} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

		typedef struct _SYSTEM_HANDLE_INFORMATION_EX
		{
			ULONG_PTR number_of_handles;
			ULONG_PTR reserved;
			SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles[ 1 ];
		} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

	public:
		/**
		* Constructor.
		*
		* @param process_id				The process ID(PID).
		* @param desired_access			The access to the process object.
		*/
		Process( DWORD process_id, DWORD desired_access );

		/**
		* Constructor.
		*
		* @param process_name			The process name.
		* @param desired_access			The access to the process object.
		*/
		Process( std::string process_name, DWORD desired_access );

		// Calls CloseOpenHandle.
		~Process( );

		// Checks if the process is running/active.
		bool IsValid( ) const;

		// Wrapper function for IsHungAppWindow, returns true if the application is hanging.
		BOOL IsNotResponding( ) const;

		/**
		* Sends the WM_NULL message to the primary window handle.
		*
		* @param timeout				The duration of the time-out period, in milliseconds.
		*/
		LRESULT IsNotResponding( UINT timeout ) const;

		/**
		* Set the access to the process object. This access right is checked against the security descriptor for the process.
		* You do not need to call this function if you need PROCESS_ALL_ACCESS as your desired access.
		*
		* @param desired_access			The access to the process object.
		*/
		void Process::SetDesiredAccess( DWORD desired_access )
		{
			m_desired_access = desired_access;
		}

		/**
		* Set the m_name member variable.
		*
		* @param module_name			The process name.
		*/
		void Process::SetModuleName( std::string module_name )
		{
			m_name = module_name;
		}

		/**
		* Set the m_hwnd member variable.
		*
		* @param window_handle			The primary window handle.
		*/
		void Process::SetPrimaryWindowHandle( HWND window_handle )
		{
			m_handle = window_handle;
		}

		// Returns m_pid member.
		DWORD &Process::GetPid( )
		{
			return m_pid;
		}

		// Returns m_desired_access member.
		DWORD &Process::GetDesiredAccess( )
		{
			return m_desired_access;
		}

		// Returns m_hwnd member.
		HWND &Process::GetPrimaryWindowHandle( )
		{
			return m_hwnd;
		}

		// Returns m_handle member.
		HANDLE &Process::GetHandle( )
		{
			return m_handle;
		}

		// Returns m_name member.
		std::string &Process::GetModuleName( )
		{
			return m_name;
		}

		/**
		* Wrapper function for OpenProcess.
		*
		* @param inherit_handle			If this value is TRUE, processes created by this process will inherit the handle.
		*/
		bool Open( BOOL inherit_handle );

		/**
		* Wrapper function for CloseHandle.
		*/
		bool CloseOpenHandle( ) const;

		/**
		* Returns process ID. -1 on failure.
		*
		* @param process_name			The target process name.
		*/
		static DWORD FetchProcessByName( std::string process_name );

		/**
		* Wrapper function for GetProcessImageFileName.
		*/
		std::string FetchProcessImageFileName( ) const;

		/**
		* Wrapper function for GetModuleFileName.
		*/
		std::string FetchModuleFileName( ) const;

		/**
		* Wrapper function for TerminateProcess.
		*
		* @param exit_code				The exit code to be used by the process and threads terminated as a result of this call.
		*/
		bool Terminate( UINT exit_code = EXIT_SUCCESS ) const;

		/**
		* Returns a handle for every thread within the process.
		*/
		std::vector<HANDLE> FetchThreads( ) const;

		/**
		* Resumes the process by enumerating all threads and calling ResumeThread.
		*/
		bool Resume( ) const;

		/**
		* EXPERIMENTAL: Resumes the process via the undocumented NtResumeProcess function.
		*/
		bool NtResume( ) const;

		/**
		* Suspends the process by enumerating all threads and calling SuspendThread.
		*/
		bool Suspend( ) const;

		/**
		* EXPERIMENTAL: Suspends the process via the undocumented NtSuspendProcess function.
		*/
		bool NtSuspend( ) const;

		// TODO: doc
		std::vector<HANDLE_INFO> FetchHandles( ) const;

		/**
		* Opens the access token associated with the process.
		*
		* @param desired_access		    Specifies an access mask that specifies the requested types of access to the access token.
		*/
		HANDLE FetchAccessToken( DWORD desired_access ) const;

		/**
		* Wrapper function for AdjustTokenPrivileges.
		*
		* @param name					A pointer to a null-terminated string that specifies the name of the privilege, as defined in the Winnt.h header file
		* @param enable_privilege		Enables or disables the privilege.
		*/
		bool SetPrivilege( LPCTSTR name, BOOL enable_privilege ) const;

		/**
		* EXPERIMENTAL: Enables or disables a privilege from the calling thread or process.
		*
		* @param privilege				Privilege index to change.
		* @param enable					If TRUE, then enable the privilege otherwise disable.
		* @param current_thread			If TRUE, then enable in calling thread, otherwise process.
		* @param enabled				Whether privilege was previously enabled or disabled.
		*/
		static bool RtlAdjustPrivileges( ULONG privilege, BOOLEAN enable, BOOLEAN current_thread, PBOOLEAN enabled );

		/**
		* Load a dynamic link library into the target process.
		*
		* @param library_path			Full path to library.
		*/
		bool LoadLibraryExternal( std::string library_path ) const;

	protected:
		DWORD m_pid;									// Process ID
		DWORD m_desired_access;							// Desired access
		HWND m_hwnd;									// Window handle
		HANDLE m_handle;								// Handle to process
		std::string m_name;								// Module name

	private:
		// Calls GetWindowThreadProcessId to return the primary window handle.
		HWND FetchPrimaryWindowHandle( );

		// Run-time dynamic linking.
		using NtSuspendProcess = NTSTATUS( NTAPI* )( HANDLE );
		using NtResumeProcess = NTSTATUS( NTAPI* )( HANDLE );
		using NtQuerySystemInformation = NTSTATUS( NTAPI* )( ULONG, PVOID, ULONG, PULONG );
		using NtQueryObject = NTSTATUS( NTAPI* )( ULONG, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG );

		using RtlSetProcessIsCritical = NTSTATUS( WINAPI* )( BOOLEAN, BOOLEAN, BOOLEAN );
		using RtlAdjustPrivilege = NTSTATUS( WINAPI* )( ULONG, BOOLEAN, BOOLEAN, PBOOLEAN );

		enum _SYSTEM_INFORMATION_CLASS
		{
			SystemProcessInformation = 0x0005,
			SystemHandleInformation = 0x0010,
			SystemExtendedProcessInformation = 0x0039,
			SystemExtendedHandleInformation = 0x0040 // SystemExtendedHandleInformation isn't stuck with 16bit process IDs.
		};
	};
}