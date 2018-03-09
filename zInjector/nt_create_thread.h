#pragma once

#include "includes.h"

struct NTCREATETHREAD_BUFFER  {
	ULONG     m_size;
	ULONG     m_unknown1;
	ULONG     m_unknown2;
	PULONG    m_unknown3;
	ULONG     m_unknown4;
	ULONG     m_unknown5;
	ULONG     m_unknown6;
	PULONG    m_unknown7;
	ULONG     m_unknown8;
}; 

typedef NTSTATUS ( __stdcall *NTCREATETHREADEX) (
	PHANDLE thread,
	ACCESS_MASK desired_access,
	LPVOID object_attributes,
	HANDLE process_handle,
	LPTHREAD_START_ROUTINE start_address,
	LPVOID lp,
	BOOL create_suspended,
	ULONG stack_zero_bits,
	ULONG size_of_stack_commit,
	ULONG size_of_stack_reserve,
	LPVOID bytes_buffer );

namespace injection_methods {
	bool nt_create_thread( wpm::Process process, const std::string &library_path );
}
