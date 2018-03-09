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

#include "main.h"
#include "utilities.h"

#include "nt_create_thread.h"

bool initialize( const std::string &base_directory, const std::string &dll_path, const std::string &process_name, int injection_method );
bool do_injection_method( const wpm::Process &process, const std::string &dll_path, int injection_method );

int main( int argc, char *argv[] ) {
	if( argc != 4 ) {
		std::cout << "Usage: " << argv[ 0 ] << " [DLL] [Process Name] [Injection Method]\n";
		std::cin.get();
		return 1;
	}

	try {
		if( !initialize( argv[ 0 ], argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) ) ) {
			utilities::raise_error();
			return 1;
		}
	}
	catch( ... ) {
		std::cout << "Unknown injection error. Are you using the correct arguments?\n";
		std::cin.get();
		return 1;
	}

	return 0;
}

bool initialize( const std::string &base_directory, const std::string &dll_path, const std::string &process_name, int injection_method ) {
	auto local_process = wpm::Process( GetCurrentProcessId( ), PROCESS_ALL_ACCESS );
	
	auto process = wpm::Process( process_name, PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ );
	if( !process.is_valid() ) {
		std::cout << "Target process '" << process_name << "' is not running.\n";
		return false;
	}

	if( !process.open( FALSE ) || !local_process.open( FALSE ) ) {
		std::cout << "Failed to open process with error code: ";
		return false;
	}

	// make sure our process has the correct privileges
	if( !local_process.set_privilege( SE_DEBUG_NAME, TRUE ) ) {
		std::cout << "Unable to set privilege with error code: ";
		return false;
	}

	auto pe = wpm::PortableExecutable( &process );
	if( !utilities::is_valid_library( pe, dll_path ) ) {
		std::cout << "Unable to retrieve image header with error code: ";
		return false;
	}

	// attempt to inject
	if( !do_injection_method( process, dll_path, injection_method ) )
		return false;

	std::string filename = dll_path + ".bat";
	if( !std::experimental::filesystem::exists( filename ) ) {
		std::cout << "Create a batch file to easily run this program with the given arguments? [Y/N]" << std::endl;

		char input;
		std::cin >> input;

		if( input == 'y' || input == 'Y' ) {
			std::ofstream of_stream( filename, std::ofstream::out );
			of_stream << base_directory << " " << dll_path << " " << process_name << " " << injection_method;
		}
	}

	return true;
}

bool do_injection_method( const wpm::Process &process, const std::string &dll_path, int injection_method ) {
	switch( injection_method ) {
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
		case METHOD_CREATEREMOTETHREAD: {
			if( !process.load_library_external( dll_path ) )
				return false;
		}
		break;
		// undocumented function
		case METHOD_NTCREATETHREAD: {
			if( !injection_methods::nt_create_thread( process, dll_path ) )
				return false;
		}
		break;
		default:
			std::cout << "Invalid injection method." << std::endl;
			std::cout << "Available injection methods:" << std::endl << "1 - CreateRemoteThread" << std::endl << "2 - NtCreateThreadEx" << std::endl;
		break;
	}

	std::cout << "Injection successful." << std::endl;

	return true;
}
