#include "nt_create_thread.h"

bool injection_methods::nt_create_thread( wpm::Process process, const std::string &library_path ) {
	auto process_handle = process.get_handle();

	LPVOID base_address = VirtualAllocEx( process_handle, nullptr, library_path.length() + 1, MEM_COMMIT, PAGE_READWRITE );
	if( !WriteProcessMemory( process_handle, base_address, library_path.c_str(), library_path.size() + 1, nullptr ) ) {
		VirtualFreeEx( process_handle, base_address, 0, MEM_RELEASE );
		return false;
	}

	auto nt_create_thread_address = reinterpret_cast<NTCREATETHREADEX>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtCreateThreadEx" ) );
	auto load_library_address = reinterpret_cast<LPTHREAD_START_ROUTINE>( GetProcAddress( GetModuleHandleA( "kernel32" ), "LoadLibraryA" ) );
	
	DWORD temp1 = 0;

	NTCREATETHREAD_BUFFER buffer{};
	buffer.m_size = sizeof( struct NTCREATETHREAD_BUFFER );
	buffer.m_unknown1 = 0x10003;
	buffer.m_unknown2 = 0x8;
	buffer.m_unknown3 = nullptr;
	buffer.m_unknown4 = 0;
	buffer.m_unknown5 = 0x10004;
	buffer.m_unknown6 = 0x4;
	buffer.m_unknown7 = &temp1;
	buffer.m_unknown8 = 0;

	HANDLE remote_thread = nullptr;
	NTSTATUS status = nt_create_thread_address( &remote_thread, 0x1FFFFF, nullptr, process_handle, load_library_address, base_address, 0, 0, 0, 0, &buffer );

	if( remote_thread == nullptr || NT_ERROR( status ) )
		return false;

	WaitForSingleObject( remote_thread, 5000 ); 
	CloseHandle( remote_thread );
	VirtualFreeEx( remote_thread, base_address, 0, MEM_RELEASE );

	return true;
}
