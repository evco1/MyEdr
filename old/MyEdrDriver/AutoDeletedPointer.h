#pragma once

#include "move.h"
#include "Debug.h"

#include <wdm.h>

#define MY_EDR_DATA_DEFAULT_TAG 'ddem'

template <typename DataType, UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG>
class AutoDeletedPointer final
{
public:
	constexpr AutoDeletedPointer() ;
	AutoDeletedPointer(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer(AutoDeletedPointer&& other);
	AutoDeletedPointer(DataType* data);

	~AutoDeletedPointer();

	AutoDeletedPointer& operator=(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer& operator=(AutoDeletedPointer&& other);
	AutoDeletedPointer& operator=(DataType* data);

	const DataType& operator*() const;
	DataType& operator*();

	const DataType* operator->() const;
	DataType* operator->();

	NTSTATUS allocate(size_t dataSize = sizeof(DataType));
	bool isAllocated() const;

	const DataType* get() const;
	DataType* get();

	void free();

	DataType* release();

private:
	DataType* m_data;
};

template<typename DataType, UINT32 Tag>
constexpr AutoDeletedPointer<DataType, Tag>::AutoDeletedPointer() :
	m_data(nullptr)
{
}

template<typename DataType, UINT32 Tag>
AutoDeletedPointer<DataType, Tag>::AutoDeletedPointer(AutoDeletedPointer&& other) :
	m_data(other.m_data)
{
	other.m_data = nullptr;
}

template<typename DataType, UINT32 Tag>
AutoDeletedPointer<DataType, Tag>::AutoDeletedPointer(DataType* data) :
	m_data(data)
{
}

template<typename DataType, UINT32 Tag>
AutoDeletedPointer<DataType, Tag>::~AutoDeletedPointer()
{
	free();
}

template<typename DataType, UINT32 Tag>
AutoDeletedPointer<DataType, Tag>& AutoDeletedPointer<DataType, Tag>::operator=(AutoDeletedPointer&& other)
{
	if (this != &other)
	{
		free();
		m_data = other.m_data;
		other.m_data = nullptr;
	}

	return *this;
}

template<typename DataType, UINT32 Tag>
AutoDeletedPointer<DataType, Tag>& AutoDeletedPointer<DataType, Tag>::operator=(DataType* data)
{
	if (m_data != data)
	{
		free();
		m_data = data;
	}

	return *this;
}

template<typename DataType, UINT32 Tag>
const DataType& AutoDeletedPointer<DataType, Tag>::operator*() const
{
	return *m_data;
}

template<typename DataType, UINT32 Tag>
DataType& AutoDeletedPointer<DataType, Tag>::operator*()
{
	return *m_data;
}

template<typename DataType, UINT32 Tag>
const DataType* AutoDeletedPointer<DataType, Tag>::operator->() const
{
	return m_data;
}

template<typename DataType, UINT32 Tag>
DataType* AutoDeletedPointer<DataType, Tag>::operator->()
{
	return m_data;
}

template<typename DataType, UINT32 Tag>
NTSTATUS AutoDeletedPointer<DataType, Tag>::allocate(size_t dataSize)
{
	if (isAllocated())
	{
		return STATUS_SUCCESS;
	}
	
	m_data = static_cast<DataType*>(ExAllocatePoolZero(NonPagedPool, dataSize, Tag));

	if (nullptr == m_data) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	return STATUS_SUCCESS;
}

template<typename DataType, UINT32 Tag>
bool AutoDeletedPointer<DataType, Tag>::isAllocated() const
{
	return m_data != nullptr;
}

template<typename DataType, UINT32 Tag>
const DataType* AutoDeletedPointer<DataType, Tag>::get() const
{
	return m_data;
}

template<typename DataType, UINT32 Tag>
DataType* AutoDeletedPointer<DataType, Tag>::get()
{
	return m_data;
}

template<typename DataType, UINT32 Tag>
void AutoDeletedPointer<DataType, Tag>::free()
{
	if (!isAllocated())
	{
		return;
	}

	ExFreePoolWithTag(m_data, Tag);
	m_data = nullptr;
}

template<typename DataType, UINT32 Tag>
DataType* AutoDeletedPointer<DataType, Tag>::release()
{
	DataType* data = m_data;
	m_data = nullptr;
	return data;
}
