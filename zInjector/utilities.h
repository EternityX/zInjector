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
#include "utilities.h"

#pragma comment (lib, "Shlwapi.lib")

#define METHOD_CREATEREMOTETHREAD 1

void RaiseError( const char *fmt, ... );
uint32_t GrabProcessByName( char *process_name );
bool CreateRemoteThreadMethod( uint32_t pid, const char *dll_path );

#endif // utilities_H
