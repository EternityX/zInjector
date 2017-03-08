#ifndef utilities_H
#define utilities_H

#pragma once

#include <windows.h>
#include <assert.h>
#include <tlhelp32.h>
#include <iostream>
#include <excpt.h>
#include <signal.h>
#include <shlwapi.h>
#include <string>
#include "utilities.h"

#pragma comment (lib, "Shlwapi.lib")

#define METHOD_CREATEREMOTETHREAD 1

void RaiseError( );
PIMAGE_NT_HEADERS RetrieveImageHeader( HANDLE map_view );
bool CheckImage( char *dll_path );
unsigned int GrabProcessByName( char *process_name );
bool CreateRemoteThreadMethod( unsigned int pid, const char *dll_path );

#endif // utilities_H
