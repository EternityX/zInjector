#pragma once

#include "includes.h"

namespace Utilities
{
	void RaiseError( );
	PIMAGE_NT_HEADERS RetrieveImageHeader( std::string dll_path );
	bool IsValidLibrary( PortableExecutable pe, std::string dll_path );
}
