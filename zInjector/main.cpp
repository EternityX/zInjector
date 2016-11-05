#include "utilities.h"

void InjectDLL(char* dll_path, char* process_name);

int main(int argc, char* argv[]) {
	assert(argc == 3);

	__try {
		InjectDLL(argv[1], argv[2]);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		RaiseError("Injection failed.");
	}

	return 0;
}

bool CreateRemoteThread(unsigned int pid, const char* dll_path) {
	LPVOID loadlib;

	HANDLE process;
	LPVOID memory;

	if (!pid) {
		return false;
	}

	process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	loadlib = LPVOID(GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"));
	memory = LPVOID(VirtualAllocEx(process, nullptr, strlen(dll_path) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

	WriteProcessMemory(process, LPVOID(memory), dll_path, strlen(dll_path) + 1, nullptr);
	CreateRemoteThread(process, nullptr, NULL, LPTHREAD_START_ROUTINE(loadlib), LPVOID(memory), NULL, nullptr);

	CloseHandle(process);
	VirtualFreeEx(process, LPVOID(memory), 0, MEM_RELEASE);

	return true;
}

void InjectDLL(char* dll_path, char* process_name) {
	unsigned int pid = GrabProcess(process_name);
	if (!pid) {
		RaiseError("Process is not running.");
		return;
	}

	if (FILE *file = fopen(dll_path, "r")) {
		fclose(file);
	}
	else {
		RaiseError("library does not exist.");
		return;
	}

	CreateRemoteThread(pid, dll_path);
}