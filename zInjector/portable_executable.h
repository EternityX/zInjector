/* MIT License

Copyright(c) 2017 (https://github.com/EternityX/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE. */

#pragma once

#include "process.h"

namespace wpm {
	class PortableExecutable {
	public:
		struct IMPORT_INFO {
			DWORD m_library;
			DWORD m_function;
		};

	public:
		/**
		* Constructor.
		*
		* @param process				The process object.
		*/
		explicit PortableExecutable( Process *process );

		/**
		* Maps a view of a file mapping into the address space of a calling process for use with FetchImageHeader.
		*/
		HANDLE create_map_view();

		/**
		* Retrieves the DOS header for use with FetchImageHeader.
		*
		* @param map_view				Handle to the map view.
		*/
		static PIMAGE_DOS_HEADER fetch_dos_header( HANDLE map_view );

		/**
		* Retrieves PIMAGE_NT_HEADERS structure
		*/
		PIMAGE_NT_HEADERS fetch_image_header();

		/**
		* Determines if the process is 64bit architecture by checking the file header for IMAGE_FILE_MACHINE_AMD64.
		*/
		bool is_64_bit();

		// TODO: doc
		std::vector<IMPORT_INFO> fetch_imports();

	protected:
		HANDLE m_file = nullptr;	// Handle to file

	private:
		Process *m_process;
	};
}