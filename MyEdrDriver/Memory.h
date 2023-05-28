#pragma once

#include <wdm.h>

#define MY_EDR_DATA_DEFAULT_TAG 'ddem'

template<typename DataType = void, UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG>
DataType* DefaultAllocate(size_t size)
{
    return static_cast<DataType*>(ExAllocatePoolZero(NonPagedPool, size, Tag));
}

template<typename DataType = void, UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG>
void DefaultDelete(DataType* pointer)
{
    ExFreePoolWithTag(pointer, Tag);
}
