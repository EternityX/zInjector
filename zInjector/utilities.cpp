#include "utilities.h"

void RaiseError( )
{
	if ( GetLastError( ) == ERROR_SUCCESS )
		return;

	LPSTR buffer = nullptr;

	// FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
	size_t size = FormatMessageA( 0x100 | 0x1000 | 0x200, nullptr, GetLastError( ), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast< LPSTR >( &buffer ), 0, nullptr );

	std::string text( buffer, size );
	std::cout << "ERROR: " << text << std::endl;
	std::cout << "Press ENTER to continue . . .";
	std::cin.get( );
}

PIMAGE_NT_HEADERS RetrieveImageHeader( HANDLE map_view )
{
	// retrieve pe
	PIMAGE_DOS_HEADER dosHeader = static_cast< PIMAGE_DOS_HEADER >( map_view );
	if ( dosHeader->e_magic != IMAGE_DOS_SIGNATURE )
		return nullptr;

	PIMAGE_NT_HEADERS imageHeader = reinterpret_cast< PIMAGE_NT_HEADERS >( reinterpret_cast< char * >( dosHeader ) + dosHeader->e_lfanew );
	if ( imageHeader->Signature != IMAGE_NT_SIGNATURE )
		return nullptr;

	if ( !( imageHeader->FileHeader.Characteristics & IMAGE_FILE_DLL ) )
	{
		std::cout << "Selected payload is not a valid DLL." << std::endl;
		return nullptr;
	}

	if ( !imageHeader->OptionalHeader.AddressOfEntryPoint )
		std::cout << "WARNING: No entry point found!" << std::endl;

	return imageHeader;
}

bool CheckImage( char *dll_path )
{
	// make sure the library exists
	FILE *stream;
	if ( fopen_s( &stream, dll_path, "r" ) == 0 )
	{
		fclose( stream );
	}
	else
	{
		return false;
	}

	HANDLE file = CreateFileA( dll_path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( file == INVALID_HANDLE_VALUE )
		return false;

	HANDLE map = CreateFileMappingA( file, nullptr, PAGE_READONLY, 0, 0, nullptr );
	if ( !map )
		return false;

	HANDLE mapView = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
	if ( !mapView )
		return false;

	PIMAGE_NT_HEADERS header = RetrieveImageHeader( mapView );
	if ( !header )
		return false;

	std::cout << "Image signature: " << header->Signature << std::endl;
	std::cout << "Image base: " << header->OptionalHeader.ImageBase << std::endl;
	std::cout << "Entry point: " << header->OptionalHeader.AddressOfEntryPoint << std::endl;
	std::cout << "DLL characteristics: " << header->OptionalHeader.DllCharacteristics << std::endl;

	if ( !CloseHandle( file ) )
		return false;

	return true;
}

/// <summary>
/// Grab process by Name (e.g. notepad.exe)
/// </summary>
/// <param name="process_name">The process name</param>
/// <returns>Process ID (PID)</returns>
unsigned int GrabProcessByName( char *process_name )
{
	HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	unsigned int count = 0;
	unsigned int pid = 0;

	if ( snap == INVALID_HANDLE_VALUE )
	{
		RaiseError( );
	}

	// make sure process is running
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

	if ( !CloseHandle( snap ) )
		RaiseError( );

	return pid;
}

/// <summary>
/// Start CreateRemoteThread injection
/// </summary>
/// <param name="pid">The process ID to our target</param>
/// <param name="dll_path">The absolute path to the dynamic link library</param>
/// <returns>Will return false on failure. Use RaiseError( ) to retrieve the error message.</returns>
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