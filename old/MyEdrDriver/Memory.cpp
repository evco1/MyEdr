#include "AutoDeletedPointer.h"

#include <wdm.h>

#pragma once

using DummyType = int;

void* __cdecl operator new(size_t size)
{
    AutoDeletedPointer<DummyType> pointer;
    pointer.allocate(size);
    return pointer.release();
}

void __cdecl operator delete(void* ptr, size_t size)
{
    UNREFERENCED_PARAMETER(size);

    AutoDeletedPointer(reinterpret_cast<DummyType*>(ptr));
}
