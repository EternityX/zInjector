#include "portable_executable.h"

namespace wpm {
	PortableExecutable::PortableExecutable( Process *process ) {
		m_process = process;
	}

	HANDLE PortableExecutable::create_map_view() {
		TCHAR buffer[ MAX_PATH ];
		if( !K32GetModuleFileNameExA( m_process->get_handle( ), nullptr, buffer, MAX_PATH ) )
			return nullptr;

		m_file = CreateFileA( buffer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
		if( m_file == INVALID_HANDLE_VALUE )
			return nullptr;

		HANDLE map = CreateFileMappingA( m_file, nullptr, PAGE_READONLY, 0, 0, nullptr );
		if( !map )
			return nullptr;

		HANDLE map_view = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
		if( !map_view )
			return nullptr;

		if( !CloseHandle( m_file ) && !CloseHandle( map ) && !CloseHandle( map_view ) )
			return nullptr;

		return map_view;
	}

	PIMAGE_DOS_HEADER PortableExecutable::fetch_dos_header( HANDLE map_view ) {
		auto dos_header = static_cast<PIMAGE_DOS_HEADER>( map_view );
		if( dos_header->e_magic != IMAGE_DOS_SIGNATURE )
			return nullptr;

		return dos_header;
	}

	PIMAGE_NT_HEADERS PortableExecutable::fetch_image_header() {
		auto dos_header = PortableExecutable::fetch_dos_header( PortableExecutable::create_map_view( ) );
		if( !dos_header )
			return nullptr;

		auto image_header = reinterpret_cast<PIMAGE_NT_HEADERS>( reinterpret_cast<PCHAR>( dos_header ) + dos_header->e_lfanew );
		if( image_header->Signature != IMAGE_NT_SIGNATURE )
			return nullptr;

		return image_header;
	}
}
