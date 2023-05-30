#pragma once

#include <ntddk.h>

const UINT32 MY_EDR_ALLOCATED_DATA_POOL_TAG = 'daem';

template <class DataType = void>
DataType* allocate(size_t size)
{
    return static_cast<DataType*>(ExAllocatePoolZero(NonPagedPool, size, MY_EDR_ALLOCATED_DATA_POOL_TAG));
}

template <class DataType = void>
void free(DataType* pointer)
{
    ExFreePoolWithTag(pointer, MY_EDR_ALLOCATED_DATA_POOL_TAG);
}

template <class DataType>
void defaultDelete(DataType* data)
{
    delete data;
}
