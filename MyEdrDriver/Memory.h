#pragma once

#include <wdm.h>

const UINT32 MY_EDR_DATA_TAG = 'ddem';

template<typename DataType = void>
DataType* allocate(size_t size)
{
    return static_cast<DataType*>(ExAllocatePoolZero(NonPagedPool, size, MY_EDR_DATA_TAG));
}

template<typename DataType = void>
void free(DataType* pointer)
{
    ExFreePoolWithTag(pointer, MY_EDR_DATA_TAG);
}

template<typename DataType>
void defaultDelete(DataType* data)
{
    delete data;
}
