#pragma once

#include "includes.h"

struct NTCREATETHREAD_BUFFER 
{
	ULONG size;
	ULONG unknown1;
	ULONG unknown2;
	PULONG unknown3;
	ULONG unknown4;
	ULONG unknown5;
	ULONG unknown6;
	PULONG unknown7;
	ULONG unknown8;
}; 

using NTCREATETHREADEX = NTSTATUS( NTAPI* ) (
	PHANDLE thread,
	ACCESS_MASK desired_access,
	LPVOID object_Attributes,
	HANDLE process_handle,
	LPTHREAD_START_ROUTINE start_address,
	LPVOID lp,
	BOOL create_suspended,
	ULONG stack_zero_bits,
	ULONG size_of_stack_commit,
	ULONG size_of_stack_reserve,
	LPVOID bytes_buffer );

namespace injection_methods
{
	bool NtCreateThread( Process process, std::string library_path );
}
