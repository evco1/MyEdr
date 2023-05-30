#include <Windows.h>

#include <iostream>
#include <string>

void main()
{
	HANDLE a = CreateFileW(
		L"\\\\.\\MyEdrDriverIoCtl",
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);
}
