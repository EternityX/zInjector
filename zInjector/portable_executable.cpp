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

	bool PortableExecutable::is_64_bit() {
		PIMAGE_NT_HEADERS nt_header = PortableExecutable::fetch_image_header( );
		if( !nt_header )
			return false;

		return nt_header->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64;
	}

	std::vector<PortableExecutable::IMPORT_INFO> PortableExecutable::fetch_imports() {
		std::vector<IMPORT_INFO> return_imports;

		HANDLE file = CreateFileA( m_process->fetch_module_file_name().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
		if( file == INVALID_HANDLE_VALUE ) {
			CloseHandle( file );
			return {};
		}
		CloseHandle( file );

		auto nt_header = PortableExecutable::fetch_image_header();
		if( !nt_header )
			return {};

		auto section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>( reinterpret_cast<DWORD>( nt_header ) + sizeof( IMAGE_NT_HEADERS ) );

		DWORD section = 0;

		DWORD rva = nt_header->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress;
		if( !rva )
			return {};

		do {
			section_header++;
			section++;
		} while ( section < nt_header->FileHeader.NumberOfSections && section_header->VirtualAddress <= rva );

		section_header--;

		DWORD base = reinterpret_cast<DWORD>( PortableExecutable::create_map_view() ) + section_header->PointerToRawData;
		auto import_descriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>( base + ( rva - section_header->VirtualAddress ) );

		IMPORT_INFO import_info{};
		while( import_descriptor->Name != 0 ) {
			import_info.m_library = base + ( import_descriptor->Name - section_header->VirtualAddress );

			auto thunk_data = reinterpret_cast<PIMAGE_THUNK_DATA>( base + ( import_descriptor->FirstThunk - section_header->VirtualAddress ) );
			while( thunk_data->u1.AddressOfData != 0 ) {
				// need to get ordinals too
				if( !( thunk_data->u1.AddressOfData & IMAGE_ORDINAL_FLAG ) )
					import_info.m_function = base + ( thunk_data->u1.AddressOfData - section_header->VirtualAddress + 2 );

				return_imports.push_back( import_info );

				thunk_data++;
			}

			import_descriptor++;
		}

		return return_imports;
	}
}
