#pragma once

#include <windows.h>
#include <assert.h>
#include <tlhelp32.h>
#include <iostream>
#include <excpt.h>
#include <signal.h>
#include "utilities.h"

void RaiseError(const char* fmt, ...);
unsigned int GrabProcess(char* process_name);