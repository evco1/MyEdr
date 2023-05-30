#include <Windows.h>

#include <memory>
#include <string>
#include <algorithm>

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

	MyEdrBlacklistProcess myEdrBlacklistProcess = { L"calculatorapp.exe" };

	if (!DeviceIoControl(deviceHandle, SYSCTL_ADD_BLACK, &myEdrBlacklistProcess, sizeof(MyEdrBlacklistProcess), nullptr, 0, nullptr, nullptr)) {
		printf("Failed to get driver's version: %u\n", GetLastError());
		return;
	}

	HANDLE stdInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	MyEdrEvent myEdrEvent;

	// Run every 100 ms until the user press any key
	do {
		DWORD readDataLength;

		while (DeviceIoControl(
			deviceHandle,
			SYSCTL_GET_EVENTS,
			nullptr,
			0,
			&myEdrEvent,
			sizeof(MyEdrEvent),
			nullptr,
			nullptr
		)) {
			printf("Event: %u, %u, %llu, %S\n", myEdrEvent.Id, myEdrEvent.ProcessId, myEdrEvent.TimeStamp, myEdrEvent.Name);

			if (FileWrite == myEdrEvent.Id)
			{
				std::wstring fullName = myEdrEvent.Name;
				std::transform(fullName.begin(), fullName.end(), fullName.begin(), std::tolower);
				size_t lastBackslashIndex = fullName.rfind(L"\\");
				std::wstring fileName;

				if (std::wstring::npos == lastBackslashIndex)
				{
					fileName = fullName;
				}
				else
				{
					fileName = fullName.c_str() + lastBackslashIndex + 1;
				}

				if (L"ransomware.kki" == fileName)
				{
					HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, myEdrEvent.ProcessId);

					if (nullptr != processHandle) {
						TerminateProcess(processHandle, 0);
						CloseHandle(processHandle);
					}
				}
			}
		}
	} while (WAIT_TIMEOUT == WaitForSingleObject(stdInputHandle, 100));
}
