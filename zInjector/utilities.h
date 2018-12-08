#pragma once

#include "includes.h"

namespace utilities {
	void raise_error();

	/**
	* Retrieves the portable executable(PE) header of the passed DLL.
	*
	* @param dll_path				Absolute path to the DLL.
	*/
	PIMAGE_NT_HEADERS retrieve_image_header( const std::string &dll_path );

	/**
	* Verifies the file exists and is a valid DLL.
	*
	* @param dll_path				Absolute path to the DLL.
	*/
	bool is_valid_library( wpm::PortableExecutable pe, const std::string &dll_path );
}
