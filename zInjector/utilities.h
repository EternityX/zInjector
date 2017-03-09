#ifndef utilities_H
#define utilities_H

#pragma once

#include "includes.h"

#define METHOD_CREATEREMOTETHREAD 1

namespace Utilities
{
	void RaiseError( );
	PIMAGE_NT_HEADERS RetrieveImageHeader( std::string dll_path );
	bool VerifyLibrary( std::string dll_path );
	unsigned int GrabProcessByName( std::string process_name );
}

#endif // utilities_H
