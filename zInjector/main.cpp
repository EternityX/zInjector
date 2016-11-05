#include "utilities.h"

void InjectDLL(char* dll_path, char* process_name, int injection_method);

int main(int argc, char* argv[]) {
	assert(argc == 4);

	auto method = atoi(argv[3]);

	__try {
		InjectDLL(argv[1], argv[2], method);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		RaiseError("Injection failed.");
	}

	return 0;
}

bool StartInjectionMethod(unsigned int pid, char* process_name, char* dll_path, int injection_method) {
	switch(injection_method) {	
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682437(v=vs.85).aspx
		case METHOD_CREATEREMOTETHREAD:
			if (!CreateRemoteThreadMethod(pid, dll_path)) {
				return false;
			}
		break;
	}

	return true;
}

void InjectDLL(char* dll_path, char* process_name, int injection_method) {
	// check if process is running
	unsigned int pid = GrabProcessByName(process_name);
	if (!pid) {
		RaiseError("Process is not running.");
		return;
	}

	// check if dll exists on disk
	if (FILE *file = fopen(dll_path, "r")) {
		fclose(file);
	}
	else {
		RaiseError("library does not exist.");
		return;
	}

	bool successful = StartInjectionMethod(pid, process_name, dll_path, injection_method);
	if (!successful) {
		RaiseError("Failed to inject library.");
	}
}