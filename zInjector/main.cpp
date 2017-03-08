#include "utilities.h"
#include "createremotethread.h"

void Initialize( char *dll_path, char *process_name, int injection_method );
bool StartInjectionMethod( unsigned int pid, char *dll_path, int injection_method );

int main( int argc, char* argv[ ] )
{
	if ( argc < 4 )
	{
		std::cout << "Usage: " << argv[ 0 ] << " [DLL] [Process Name] [Injection Method]" << std::endl;
		std::cin.get( );

		return EXIT_FAILURE;
	}

	__try
	{
		Initialize( argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		std::cout << "Unknown injection error. Are you using the correct arguments?" << std::endl;
		std::cin.get( );

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void Initialize( char *dll_path, char *process_name, int injection_method )
{
	// make sure process is running
	unsigned int pid = Utilities::GrabProcessByName( process_name );
	if ( !pid )
	{
		std::cout << "Target process '" << process_name << "' is not running." << std::endl;
		return;
	}

	if ( !Utilities::VerifyLibrary( dll_path ) )
	{
		Utilities::RaiseError( );
		return;
	}

	// try to inject
	if ( !StartInjectionMethod( pid, dll_path, injection_method ) )
	{
		Utilities::RaiseError( );
	}
}

bool StartInjectionMethod( unsigned int pid, char *dll_path, int injection_method )
{
	switch ( injection_method )
	{
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
	case METHOD_CREATEREMOTETHREAD:
	{
		if ( !CreateRemoteThreadMethod( pid, dll_path ) )
		{
			return false;
		}
		std::cout << "Injection successful (probably)." << std::endl;
	}
	break;
	default:
		std::cout << "Invalid injection method." << std::endl;
		std::cout << "Available injection methods:" << std::endl << "1 - CreateRemoteThread" << std::endl;
		break;
	}

	return true;
}