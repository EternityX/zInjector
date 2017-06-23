#include "utilities.h"
#include "main.h"

void Utilities::RaiseError( )
{
	DWORD last_error = GetLastError( );
	if ( last_error == ERROR_SUCCESS ) {
		return;
	}

	nullptr_t format_buffer;
	auto format_flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD size = FormatMessageA( format_flags, nullptr, last_error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast<LPSTR>( &format_buffer ), 0, nullptr );

	std::string text{ format_buffer, size };
	std::cout << text << std::endl;
	std::cout << "Press ENTER to continue . . .";
	std::cin.get( );
}

/// <summary>
/// Retrieves the portable executable(PE) header of the passed DLL
/// </summary>
/// <param name="dll_path">Absolute path to the DLL</param>
/// <returns>Pointer to PIMAGE_NT_HEADERS</returns>
PIMAGE_NT_HEADERS Utilities::RetrieveImageHeader( std::string dll_path )
{
	// NOTE: https://en.wikibooks.org/wiki/X86_Disassembly/Windows_Executable_Files

	// The handle to our module
	HANDLE file = CreateFileA( dll_path.c_str( ), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( file == INVALID_HANDLE_VALUE ) {
		return nullptr;
	}

	// Create a mapping object for our module
	HANDLE map = CreateFileMappingA( file, nullptr, PAGE_READONLY, 0, 0, nullptr );
	if ( !map ) {
		return nullptr;
	}

	// Map the file with the handle to the mapping object we created
	HANDLE map_view = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
	if ( !map_view ) {
		return nullptr;
	}

	// Grab the MS-DOS header with our file map
	// https://en.wikibooks.org/wiki/X86_Disassembly/Windows_Executable_Files#MS-DOS_header
	auto dos_header = static_cast< PIMAGE_DOS_HEADER >( map_view );

	// Make sure the MS-DOS header is actually valid
	if ( dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
		return nullptr;
	}

	// Grab the NT header
	auto image_nt_headers = reinterpret_cast< PIMAGE_NT_HEADERS >( reinterpret_cast< char * >( dos_header ) + dos_header->e_lfanew );

	// Make sure it's valid
	if ( image_nt_headers->Signature != IMAGE_NT_SIGNATURE ) {
		return nullptr;
	}

	if ( !CloseHandle( file ) ) {
		return nullptr;
	}

	return image_nt_headers;
}

/// <summary>
/// Verifies the file exists and is a valid DLL
/// </summary>
/// <param name="dll_path">Absolute path to the DLL</param>
/// <returns>True on success</returns>
bool Utilities::IsValidLibrary( PortableExecutable pe, std::string dll_path )
{
	auto library_header = RetrieveImageHeader( dll_path.c_str( ) );
	if ( !library_header ) {
		return false;
	}

	if ( !( library_header->FileHeader.Characteristics & IMAGE_FILE_DLL ) )
	{
		std::cout << "The selected payload is not a valid DLL." << std::endl;
		return false;
	}
	if ( !library_header->FileHeader.Machine == pe.FetchImageHeader( )->FileHeader.Machine )
	{
		std::cout << "The selected payload's architecture must match the target's" << std::endl;
		return false;
	}

	if ( !library_header->OptionalHeader.AddressOfEntryPoint ) {
		std::cout << "ERROR: No entry point found!" << std::endl;
	}

	return true;
}