#ifndef utilities_H
#define utilities_H

#pragma once

#include "includes.h"

#define METHOD_CREATEREMOTETHREAD 1

namespace Utilities
{
	void RaiseError( );
	bool FileExists( char *path );
	PIMAGE_NT_HEADERS RetrieveImageHeader( char *dll_path );
	bool VerifyLibrary( char *dll_path );
	unsigned int GrabProcessByName( char *process_name );
}

#endif // utilities_H
