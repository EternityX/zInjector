#pragma once  

#define NOMINMAX

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <winternl.h>
#include <ntstatus.h>

#include <cstdlib>
#include <psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream> 
#include <shlwapi.h>
#include <string>
#include <vector>
#include <map>
#include <stdio.h>

#include <experimental/filesystem> 

#include "utilities.h"
#include "process.h"

#pragma comment (lib, "Shlwapi.lib")
#pragma comment(lib,"psapi")