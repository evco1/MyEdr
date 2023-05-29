#pragma once

#include <wdm.h>

const UINT32 MY_EDR_DATA_DEFAULT_TAG = 'ddem';

template<typename DataType = void, UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG>
DataType* allocate(size_t size)
{
    return static_cast<DataType*>(ExAllocatePoolZero(NonPagedPool, size, Tag));
}

template<typename DataType = void, UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG>
void free(DataType* pointer)
{
    ExFreePoolWithTag(pointer, Tag);
}
