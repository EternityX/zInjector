#include "utilities.h"

void Utilities::RaiseError( )
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

bool Utilities::FileExists( char *path )
{
	FILE *stream;
	if ( fopen_s( &stream, path, "r" ) == 0 )
	{
		fclose( stream );
	}
	else
	{
		return false;
	}

	return true;
}

/// <summary>
/// Retrieves the process header of the passed DLL
/// </summary>
/// <param name="dll_path">Absolute path to the DLL</param>
/// <returns>Pointer to PIMAGE_NT_HEADERS</returns>
PIMAGE_NT_HEADERS Utilities::RetrieveImageHeader( char *dll_path )
{
	HANDLE file = CreateFileA( dll_path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( file == INVALID_HANDLE_VALUE )
		return nullptr;

	HANDLE map = CreateFileMappingA( file, nullptr, PAGE_READONLY, 0, 0, nullptr );
	if ( !map )
		return nullptr;

	HANDLE mapView = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
	if ( !mapView )
		return nullptr;

	PIMAGE_DOS_HEADER dosHeader = static_cast< PIMAGE_DOS_HEADER >( mapView );
	if ( dosHeader->e_magic != IMAGE_DOS_SIGNATURE )
		return nullptr;

	PIMAGE_NT_HEADERS imageHeader = reinterpret_cast< PIMAGE_NT_HEADERS >( reinterpret_cast< char * >( dosHeader ) + dosHeader->e_lfanew );
	if ( imageHeader->Signature != IMAGE_NT_SIGNATURE )
		return nullptr;

	if ( !UnmapViewOfFile( mapView ) )
		return nullptr;

	if ( !CloseHandle( map ) )
		return nullptr;

	if ( !CloseHandle( file ) )
		return nullptr;

	return imageHeader;
}

/// <summary>
/// Verifies the file exists and is a valid DLL
/// </summary>
/// <param name="dll_path">Absolute path to the DLL</param>
/// <returns>True on success</returns>
bool Utilities::VerifyLibrary( char *dll_path )
{
	if ( !FileExists( dll_path ) )
		return false;

	PIMAGE_NT_HEADERS header = RetrieveImageHeader( dll_path );
	if ( !header )
		return false;

	if ( !( header->FileHeader.Characteristics & IMAGE_FILE_DLL ) )
	{
		std::cout << "Selected payload is not a valid DLL." << std::endl;
		return false;
	}

	if ( !header->OptionalHeader.AddressOfEntryPoint )
		std::cout << "WARNING: No entry point found!" << std::endl;

	return true;
}

/// <summary>
/// Grab process by name (e.g. notepad.exe)
/// </summary>
/// <param name="process_name">The process name</param>
/// <returns>Process ID (PID)</returns>
unsigned int Utilities::GrabProcessByName( char *process_name )
{
	HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	unsigned int count = 0;
	unsigned int pid = 0;

	if ( snap == INVALID_HANDLE_VALUE )
		RaiseError( );

	// make sure process is running
	if ( !WaitForSingleObject( snap, 5000 ) )
		return 0;

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
		pid = -1;

	if ( !CloseHandle( snap ) )
		RaiseError( );

	return pid;
}

