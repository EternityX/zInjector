#include "nt_create_thread.h"

bool injection_methods::NtCreateThread( Process process, std::string library_path )
{
	LPVOID base_address = VirtualAllocEx( process.GetHandle( ), nullptr, library_path.length( ) + 1, MEM_COMMIT, PAGE_READWRITE );
	if ( !WriteProcessMemory( process.GetHandle( ), base_address, library_path.c_str( ), library_path.size( ) + 1, nullptr ) )
	{
		VirtualFreeEx( process.GetHandle( ), base_address, 0, MEM_RELEASE );
		return false;
	}

	auto NtCreateThreadAddress = reinterpret_cast<NTCREATETHREADEX>( GetProcAddress( GetModuleHandleA( "ntdll" ), "NtCreateThreadEx" ) );
	auto LoadLibraryAddress = reinterpret_cast<LPTHREAD_START_ROUTINE>( GetProcAddress( GetModuleHandleA( "kernel32" ), "LoadLibraryA" ) );
	
	DWORD temp1 = 0; 

	NTCREATETHREAD_BUFFER buffer;
	buffer.size = sizeof( struct NTCREATETHREAD_BUFFER );
	buffer.unknown1 = 0x10003;
	buffer.unknown2 = 0x8;
	buffer.unknown3 = nullptr;
	buffer.unknown4 = 0;
	buffer.unknown5 = 0x10004;
	buffer.unknown6 = 4;
	buffer.unknown7 = &temp1;
	buffer.unknown8 = 0;

	HANDLE remote_thread = nullptr;
	NTSTATUS status = NtCreateThreadAddress( &remote_thread, 0x1FFFFF, nullptr, process.GetHandle( ), LoadLibraryAddress, base_address, 0, 0, 0, 0, &buffer );

	if( remote_thread == nullptr || NT_ERROR( status ) )  {
		return false;
    }

	WaitForSingleObject( remote_thread, 5000 ); 
	CloseHandle( remote_thread );
	VirtualFreeEx( remote_thread, base_address, 0, MEM_RELEASE );

	return true;
}
