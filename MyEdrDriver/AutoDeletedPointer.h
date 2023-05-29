#pragma once

#include "Memory.h"

#include <wdm.h>

#pragma warning(disable: 4180)

template <
	class DataType,
	class DeleterType = void(DataType* pointer),
	DeleterType* Deleter = defaultDelete
>
class AutoDeletedPointer final
{
public:
	AutoDeletedPointer(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer(AutoDeletedPointer&& other);
	AutoDeletedPointer(DataType* data = nullptr);

	~AutoDeletedPointer();

	AutoDeletedPointer& operator=(const AutoDeletedPointer&) = delete;
	AutoDeletedPointer& operator=(AutoDeletedPointer&& other);
	AutoDeletedPointer& operator=(DataType* data);

	bool operator==(const DataType* data) const;

 	const DataType& operator*() const;
	DataType& operator*();

	const DataType* operator->() const;
	DataType* operator->();

	void free();

	const DataType* get() const;
	DataType*& get();

	DataType* release();

private:
	DataType* m_data;
};

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, DeleterType, Deleter>::AutoDeletedPointer(AutoDeletedPointer&& other) :
	m_data{ other.m_data }
{
	other.m_data = nullptr;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, DeleterType, Deleter>::AutoDeletedPointer(DataType* data) :
	m_data{ data }
{
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, DeleterType, Deleter>::~AutoDeletedPointer()
{
	free();
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, DeleterType, Deleter>& AutoDeletedPointer<DataType, DeleterType, Deleter>::operator=(AutoDeletedPointer&& other)
{
	if (this != &other)
	{
		free();
		m_data = other.m_data;
		other.m_data = nullptr;
	}

	return *this;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
AutoDeletedPointer<DataType, DeleterType, Deleter>& AutoDeletedPointer<DataType, DeleterType, Deleter>::operator=(DataType* data)
{
	if (m_data != data)
	{
		free();
		m_data = data;
	}

	return *this;
}

template <class DataType, class DeleterType, DeleterType* Deleter>
bool AutoDeletedPointer<DataType, DeleterType, Deleter>::operator==(const DataType* data) const
{
	return data == m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
const DataType& AutoDeletedPointer<DataType, DeleterType, Deleter>::operator*() const
{
	return *m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
DataType& AutoDeletedPointer<DataType, DeleterType, Deleter>::operator*()
{
	return *m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
const DataType* AutoDeletedPointer<DataType, DeleterType, Deleter>::operator->() const
{
	return m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
DataType* AutoDeletedPointer<DataType, DeleterType, Deleter>::operator->()
{
	return m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
void AutoDeletedPointer<DataType, DeleterType, Deleter>::free()
{
	if (nullptr == m_data)
	{
		return;
	}

	Deleter(m_data);
	m_data = nullptr;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
const DataType* AutoDeletedPointer<DataType, DeleterType, Deleter>::get() const
{
	return m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
DataType*& AutoDeletedPointer<DataType, DeleterType, Deleter>::get()
{
	return m_data;
}

template <
	class DataType,
	class DeleterType,
	DeleterType* Deleter
>
DataType* AutoDeletedPointer<DataType, DeleterType, Deleter>::release()
{
	DataType* data = m_data;
	m_data = nullptr;
	return data;
}
