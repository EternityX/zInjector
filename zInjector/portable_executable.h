/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#pragma once

#include "process.h"

namespace wpm
{
	class PortableExecutable
	{
	public:
		struct IMPORT_INFO
		{
			unsigned long library;
			unsigned long function;
		};

	public:
		/**
		* Constructor.
		*
		* @param process				The process object.
		*/
		explicit PortableExecutable( Process* process );

		/**
		* Maps a view of a file mapping into the address space of a calling process for use with FetchImageHeader.
		*/
		HANDLE CreateMapView( );

		/**
		* Retrieves the DOS header for use with FetchImageHeader.
		*
		* @param map_view				Handle to the map view.
		*/
		static PIMAGE_DOS_HEADER FetchDOSHeader( HANDLE map_view );

		/**
		* Retrieves PIMAGE_NT_HEADERS structure
		*/
		PIMAGE_NT_HEADERS FetchImageHeader( );

		/**
		* Determines if the process is 64bit architecture by checking the file header for IMAGE_FILE_MACHINE_AMD64.
		*/
		bool Is64Bit( );

		// TODO: doc
		std::vector<IMPORT_INFO> FetchImports( );

	protected:
		HANDLE m_file = nullptr;	// Handle to file

	private:
		Process* m_process;
	};
}