#pragma once

#include "includes.h"

namespace Utilities
{
	void RaiseError( );
	PIMAGE_NT_HEADERS RetrieveImageHeader( std::string dll_path );
	bool VerifyLibrary( std::string dll_path );
	unsigned int GrabProcessByName( std::string process_name );
}
