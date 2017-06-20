/* MIT License

Copyright(c) 2017 (https://github.com/EternityX/zInjector)

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
#include "createremotethread.h"

void Initialize( std::string base_directory, std::string dll_path, std::string process_name, int injection_method );
bool DoInjectionMethod( Process process, std::string dll_path, int injection_method );

int main( int argc, char* argv[ ] )
{
	if ( argc != 4 )
	{
		std::cout << "Usage: " << argv[ 0 ] << " [DLL] [Process Name] [Injection Method]" << std::endl;
		std::cin.get( );

		return 1;
	}

	try
	{
		Initialize( argv[ 0 ], argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) );
	}
	catch ( ... )
	{
		std::cout << "Unknown injection error. Are you using the correct arguments?" << std::endl;
		std::cin.get( );

		return 1;
	}

	return 0;
}

void Initialize( std::string base_directory, std::string dll_path, std::string process_name, int injection_method )
{
	auto local_process = Process( GetCurrentProcessId( ), PROCESS_ALL_ACCESS );
	if ( !local_process.SetPrivilege( SE_DEBUG_NAME, TRUE ) )
	{
		Utilities::RaiseError( );
		return;
	}

	auto process = Process( process_name, PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ );
	if ( !process.IsValid( ) )
	{
		std::cout << "Target process '" << process_name << "' is not running." << std::endl;
		return;
	}

	if ( !process.Open( FALSE ) )
	{
		Utilities::RaiseError( );
		return;
	}

	if ( !Utilities::IsValidLibrary( process, dll_path ) )
	{
		Utilities::RaiseError( );
		return;
	}

	// Try to inject
	if ( !DoInjectionMethod( process, dll_path, injection_method ) ) {
		Utilities::RaiseError( );
	}

	std::string filename = dll_path + ".bat";
	if ( !std::experimental::filesystem::exists( filename ) )
	{
		std::cout << "Create a batch file to easily run this program with the given arguments? [Y/N]" << std::endl;

		char input;
		std::cin >> input;

		if ( input == 'y' || input == 'Y' )
		{
			std::ofstream of_stream( filename, std::ofstream::out );
			of_stream << base_directory << " " << dll_path << " " << process_name << " " << injection_method;
		}
	}
}

bool DoInjectionMethod( Process process, std::string dll_path, int injection_method )
{
	switch ( injection_method )
	{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
	case METHOD_CREATEREMOTETHREAD:
	{
		if ( !CreateRemoteThreadMethod( process, dll_path.c_str( ) ) ) {
			return false;
		}

		std::cout << "Injection successful." << std::endl;
	}
	break;
	default:
		std::cout << "Invalid injection method." << std::endl;
		std::cout << "Available injection methods:" << std::endl << "1 - CreateRemoteThread" << std::endl;
	break;
	}

	return true;
}
