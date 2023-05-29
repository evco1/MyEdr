#pragma once

#include "Memory.h"

#include <wdm.h>

#pragma warning(disable: 4180)

template<
	typename DataType,
	UINT32 Tag = MY_EDR_DATA_DEFAULT_TAG,
	typename DeleterType = void(DataType* pointer),
	DeleterType* Deleter = free
>
class AutoDeletedPointer final
{
public:
	AutoDeletedPointer(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer(AutoDeletedPointer&& other);
	AutoDeletedPointer(DataType* data = nullptr, DeleterType* deleter = ::free);

	~AutoDeletedPointer();

	AutoDeletedPointer& operator=(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer& operator=(AutoDeletedPointer&& other);
	AutoDeletedPointer& operator=(DataType* data);

 	const DataType& operator*() const;
	DataType& operator*();

	const DataType* operator->() const;
	DataType* operator->();

	bool isAllocated() const;
	NTSTATUS allocate(size_t dataSize = sizeof(DataType));
	void free();

	const DataType* get() const;
	DataType*& get();

	DataType* release();

private:
	DataType* m_data;
	DeleterType* m_deleter;
};

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::AutoDeletedPointer(AutoDeletedPointer&& other) :
	m_data{ other.m_data },
	m_deleter{ other.m_deleter }
{
	other.m_data = nullptr;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::AutoDeletedPointer(DataType* data, DeleterType* deleter) :
	m_data{ data },
	m_deleter{ deleter }
{
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::~AutoDeletedPointer()
{
	free();
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>& AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator=(AutoDeletedPointer&& other)
{
	if (this != &other)
	{
		free();
		m_data = other.m_data;
		m_deleter = other.m_deleter;
		other.m_data = nullptr;
	}

	return *this;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>& AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator=(DataType* data)
{
	if (m_data != data)
	{
		free();
		m_data = data;
	}

	return *this;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
const DataType& AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator*() const
{
	return *m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
DataType& AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator*()
{
	return *m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
const DataType* AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator->() const
{
	return m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
DataType* AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::operator->()
{
	return m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
bool AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::isAllocated() const
{
	return m_data != nullptr;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
NTSTATUS AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::allocate(size_t dataSize)
{
	if (isAllocated())
	{
		return STATUS_SUCCESS;
	}
	
	m_data = ::allocate<DataType, Tag>(dataSize);

	if (nullptr == m_data) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	return STATUS_SUCCESS;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
void AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::free()
{
	if (!isAllocated())
	{
		return;
	}

	if (nullptr != m_deleter)
	{
		m_deleter(m_data);
	}

	m_data = nullptr;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
const DataType* AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::get() const
{
	return m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
DataType*& AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::get()
{
	return m_data;
}

template<
	typename DataType,
	UINT32 Tag,
	typename DeleterType,
	DeleterType* Deleter
>
DataType* AutoDeletedPointer<DataType, Tag, DeleterType, Deleter>::release()
{
	DataType* data = m_data;
	m_data = nullptr;
	return data;
}
