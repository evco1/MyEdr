#include "Memory.h"

#include <wdm.h>

void* __cdecl operator new(size_t size)
{
    return DefaultAllocate(size);
}

void __cdecl operator delete(void* ptr, size_t size)
{
    UNREFERENCED_PARAMETER(size);

    DefaultDelete(ptr);
}
