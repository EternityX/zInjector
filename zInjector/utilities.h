#pragma once

#include <windows.h>
#include <assert.h>
#include <tlhelp32.h>
#include <iostream>
#include <excpt.h>
#include <signal.h>
#include <shlwapi.h>
#include "utilities.h"

#define METHOD_CREATEREMOTETHREAD 1
#define METHOD_CODECAVE 2
#define METHOD_MANUALMAP 3
#define METHOD_THREADHIJACK 4

void RaiseError(const char* fmt, ...);
unsigned int GrabProcessByName(char* process_name);
bool CreateRemoteThreadMethod(unsigned int pid, const char* dll_path);