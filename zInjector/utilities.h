#pragma once

#include "includes.h"

namespace utilities {
	void raise_error();
	PIMAGE_NT_HEADERS retrieve_image_header( const std::string &dll_path );
	bool is_valid_library( wpm::PortableExecutable pe, const std::string &dll_path );
}
