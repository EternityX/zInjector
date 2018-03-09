#pragma once  

#define NOMINMAX

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <winternl.h>
#include <ntstatus.h>

#include <cstdlib>
#include <cstdio>
#include <Psapi.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream> 
#include <Shlwapi.h>
#include <string>
#include <vector>
#include <map>

#include <experimental/filesystem> 

#include "process.h"
#include "portable_executable.h"

#include "utilities.h"

#pragma comment (lib, "Shlwapi.lib")
#pragma comment(lib,"psapi")