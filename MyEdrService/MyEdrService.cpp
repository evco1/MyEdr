#include <Windows.h>

#include <memory>

#include "../MyEdrDriver/MyEdrDriver.h"

void main()
{
	HANDLE deviceHandle = CreateFileW(
		L"\\\\.\\MyEdrDriverIoCtl",
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (INVALID_HANDLE_VALUE == deviceHandle) {
		printf("CreateFile failed : %u\n", GetLastError());
	}

	// Auto close device handle
	std::unique_ptr<
		std::remove_pointer_t<HANDLE>,
		decltype(&CloseHandle)
	> autoCloseHandle{ deviceHandle, CloseHandle };
	
	MyEdrVersion myEdrVersion;

	if (!DeviceIoControl(deviceHandle, SYSCTL_GET_VERSION, nullptr, 0, &myEdrVersion, sizeof(MyEdrVersion), nullptr, nullptr)) {
		printf("Failed to get driver's version: %u\n", GetLastError());
		return;
	}

	printf("Driver's version: %u.%u.%u\n", myEdrVersion.Major, myEdrVersion.Minor, myEdrVersion.Patch);
}
