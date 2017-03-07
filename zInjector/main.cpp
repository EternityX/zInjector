#include "utilities.h"

void InjectDLL( char *dll_path, char *process_name, int injection_method );

int main( int argc, char* argv[ ] )
{
	if ( argc != 4 )
	{
		std::cout << "Usage: " << argv[ 0 ] << " [DLL] [Process Name] [Injection Method]" << std::endl;
		std::cin.get( );
	}

	__try
	{
		InjectDLL( argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		std::cout << "Unknown injection error. Are you using the correct arguments?" << std::endl;
		std::cin.get( );
	}

	return EXIT_SUCCESS;
}

bool StartInjectionMethod( unsigned int pid, char *dll_path, int injection_method )
{
	switch ( injection_method )
	{	
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
	case METHOD_CREATEREMOTETHREAD:
		if ( !CreateRemoteThreadMethod( pid, dll_path ) )
		{
			return false;
		}
	break;
	default: 
		std::cout << "Invalid injection method." << std::endl;
		std::cout << "Available injection methods:" << std::endl << "1 - CreateRemoteThread" << std::endl;
	break;
	}

	return true;
}

void InjectDLL( char *dll_path, char *process_name, int injection_method )
{
	// check if process is running
	unsigned int pid = GrabProcessByName( process_name );
	if ( !pid )
	{
		std::cout << "Target process " << process_name << "is not running." << std::endl;
		return;
	}

	// check if library exists on disk
	FILE *stream;
	if ( fopen_s( &stream, dll_path, "r" ) == 0 )
	{
		fclose( stream );
	}
	else
	{
		std::cout << "Selected library does not exist." << std::endl;
		return;
	}

	// try to inject
	if ( !StartInjectionMethod( pid, dll_path, injection_method ) )
	{
		RaiseError( );
	}
}
