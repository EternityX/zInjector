#include "utilities.h"

void InjectDLL( char *dll_path, char *process_name, int injection_method );

int main( int argc, char* argv[ ] )
{
	if ( argc != 3 )
	{
		RaiseError( "Usage: %s [DLL] [Process Name] [Injection Type]\n", argv[ 0 ] );
	}

	__try
	{
		InjectDLL( argv[ 1 ], argv[ 2 ], atoi( argv[ 3 ] ) );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		RaiseError( "Injection failed." );
	}

	return EXIT_SUCCESS;
}

bool StartInjectionMethod( uint32_t pid, char *process_name, char *dll_path, int injection_method )
{
	switch ( injection_method )
	{
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
	case METHOD_CREATEREMOTETHREAD :
		if ( !CreateRemoteThreadMethod( pid, dll_path ) )
		{
			return false;
		}
		break;
	default: break;
	}

	return true;
}

void InjectDLL( char *dll_path, char *process_name, int injection_method )
{
	// check if process is running
	uint32_t pid = GrabProcessByName( process_name );
	if ( !pid )
	{
		RaiseError( "Process is not running." );
		return;
	}

	// check if library exists on disk
	FILE *stream;
	if ( errno_t err = fopen_s( &stream, dll_path, "r" ) == 0 )
	{
		fclose( stream );
	}
	else
	{
		RaiseError( "library does not exist." );
		return;
	}

	// try to inject
	if ( !StartInjectionMethod( pid, process_name, dll_path, injection_method ) )
	{
		RaiseError( "Failed to inject library." );
	}
}
