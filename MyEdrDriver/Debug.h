#pragma once

#include <ntddk.h>

#define DEBUG_PRINT(format, ...) DbgPrint("MyEdr:" __FILE__ ":" __FUNCTION__ ":%u " format "\n", __LINE__, __VA_ARGS__)
