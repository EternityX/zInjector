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

Process* process;

void Initialize( std::string dll_path, std::string process_name, int injection_method );
bool StartInjectionMethod( int pid, std::string dll_path, int injection_method );

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
		Initialize( argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) );
	}
	catch ( ... )
	{
		std::cout << "Unknown injection error. Are you using the correct arguments?" << std::endl;
		std::cin.get( );

		return 1;
	}

	return 0;
}

void Initialize( std::string dll_path, std::string process_name, int injection_method )
{
	unsigned int pid = Utilities::GrabProcessByName( process_name );
	if ( !pid )
	{
		std::cout << "Target process '" << process_name << "' is not running." << std::endl;
		return;
	}

	// With all possible rights, grab the handle to the target process
	process = new Process( pid );
	if ( !process->Open( PROCESS_ALL_ACCESS, FALSE ) )
	{
		Utilities::RaiseError( );
		return;
	}

	if ( !Utilities::VerifyLibrary( dll_path ) )
	{
		Utilities::RaiseError( );
		return;
	}

	// Try to inject
	if ( !StartInjectionMethod( pid, dll_path, injection_method ) )
		Utilities::RaiseError( );

	delete process;
}

bool StartInjectionMethod( int pid, std::string dll_path, int injection_method )
{
	switch ( injection_method )
	{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
	case METHOD_CREATEREMOTETHREAD:
	{
		if ( !CreateRemoteThreadMethod( pid, dll_path.c_str( ) ) )
			return false;

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