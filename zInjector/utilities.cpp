#include "utilities.h"
#include "main.h"

void utilities::raise_error() {
	DWORD last_error = GetLastError();
	if( last_error == ERROR_SUCCESS )
		return;

	nullptr_t format_buffer;
	auto format_flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD size = FormatMessageA( format_flags, nullptr, last_error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast<LPSTR>( &format_buffer ), 0, nullptr );

	std::string text{ format_buffer, size };
	std::cout << text << "\n";
	std::cout << "Press ENTER to continue . . .";
	std::cin.get();
}

PIMAGE_NT_HEADERS utilities::retrieve_image_header( const std::string &dll_path ) {
	// note: https://en.wikibooks.org/wiki/X86_Disassembly/Windows_Executable_Files

	// the handle to our module
	HANDLE file = CreateFileA( dll_path.c_str( ), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if( file == INVALID_HANDLE_VALUE )
		return nullptr;

	// create a mapping object for our module
	HANDLE map = CreateFileMappingA( file, nullptr, PAGE_READONLY, 0, 0, nullptr );
	if( !map )
		return nullptr;

	// map the file with the handle to the mapping object we created
	HANDLE map_view = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
	if( !map_view )
		return nullptr;

	// grab the ms-dos header with our file map
	// https://en.wikibooks.org/wiki/X86_Disassembly/Windows_Executable_Files#MS-DOS_header
	auto dos_header = static_cast< PIMAGE_DOS_HEADER >( map_view );

	// make sure the ms-dos header is actually valid
	if( dos_header->e_magic != IMAGE_DOS_SIGNATURE )
		return nullptr;

	// grab the nt header
	auto image_nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>( reinterpret_cast<char *>( dos_header ) + dos_header->e_lfanew );

	// make sure it's valid
	if( image_nt_headers->Signature != IMAGE_NT_SIGNATURE )
		return nullptr;

	if( !CloseHandle( file ) )
		return nullptr;

	return image_nt_headers;
}

bool utilities::is_valid_library( wpm::PortableExecutable pe, const std::string &dll_path ) {
	auto image_nt_headers = retrieve_image_header( dll_path );
	if( !image_nt_headers )
		return false;

	if( !( image_nt_headers->FileHeader.Characteristics & IMAGE_FILE_DLL ) ) {
		std::cout << "The selected payload is not a valid DLL.\n";
		return false;
	}
	if( !image_nt_headers->FileHeader.Machine == pe.fetch_image_header()->FileHeader.Machine ) {
		std::cout << "The selected payload's architecture must match the target's\n";
		return false;
	}

	if( !image_nt_headers->OptionalHeader.AddressOfEntryPoint )
		std::cout << "No entry point found!\n";

	return true;
}