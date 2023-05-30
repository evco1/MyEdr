#include <ntddk.h>

#include "Memory.h"

void* __cdecl operator new(size_t size)
{
    return allocate(size);
}

void* __cdecl operator new[](size_t size)
{
    return operator new(size);
}

void __cdecl operator delete(void* ptr, size_t size)
{
    UNREFERENCED_PARAMETER(size);

    free(ptr);
}
