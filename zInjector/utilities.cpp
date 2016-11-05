#include "utilities.h"

void RaiseError(const char* fmt, ...) {
	char buf[2048];

	va_list	args;

	va_start(args, fmt);
	vsprintf_s(buf, sizeof(buf), fmt, args);
	va_end(args);

	MessageBox(nullptr, buf, "zInjector", MB_OK | MB_ICONEXCLAMATION);
}

unsigned int GrabProcess(char* process_name) {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	unsigned int count = 0;
	unsigned int pid = 0;

	if (snap == INVALID_HANDLE_VALUE) {
		throw GetLastError();
	}

	// check if process is running
	if (!WaitForSingleObject(snap, 0) == WAIT_TIMEOUT) {
		return 0;
	}

	PROCESSENTRY32 proc;
	proc.dwSize = sizeof(PROCESSENTRY32);
	BOOL ret = Process32Next(snap, &proc);

	while (ret) {
		if (!_stricmp(proc.szExeFile, process_name)) {
			count++;
			pid = proc.th32ProcessID;
		}
		ret = Process32Next(snap, &proc);
	}

	if (count > 1) {
		pid = -1;
	}

	CloseHandle(snap);

	return pid;
}