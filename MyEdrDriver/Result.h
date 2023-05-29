#pragma once

#include "AutoDeletedPointer.h"
#include "move.h"

#include <wdm.h>

#define ENFORCE_SEMICOLON static_assert(true)

#define RETURN_ON_CONDITION(expression, returnValue)	\
if (expression) {										\
	return returnValue;									\
}														\
ENFORCE_SEMICOLON

#define RETURN_ON_BAD_STATUS(expression, returnValue) RETURN_ON_CONDITION(!NT_SUCCESS(expression), returnValue)

#define RETURN_STATUS_ON_BAD_STATUS(expression) {	\
	const NTSTATUS _status = expression;			\
	RETURN_ON_BAD_STATUS(_status, _status);			\
}													\
ENFORCE_SEMICOLON

#define RETURN_ON_BAD_RESULT(expression, returnValue) RETURN_ON_BAD_STATUS(expression.getStatus(), returnValue)

template <
	class DataType,
	class AllocatedDataType = AutoDeletedPointer<DataType>
>
class Result final
{
public:
	Result(NTSTATUS status = STATUS_SUCCESS);
	Result(const Result&) = delete;
	Result(Result&&) = default;
	Result(const DataType& data, NTSTATUS status = STATUS_SUCCESS);
	Result(DataType&& data, NTSTATUS status = STATUS_SUCCESS);

	Result& operator=(const Result&) = delete;
	Result& operator=(Result&&);

	const DataType& operator*() const;
	DataType& operator*();

	const DataType* operator->() const;
	DataType* operator->();

	NTSTATUS getStatus() const;

private:
	AllocatedDataType m_data;
	NTSTATUS m_status;
};

template <class DataType, class AllocatedDataType>
Result<DataType, AllocatedDataType>::Result(const NTSTATUS status) :
	m_status{ status }
{
}

template <class DataType, class AllocatedDataType>
Result<DataType, AllocatedDataType>::Result(const DataType& data, NTSTATUS status)
{
	m_data = new DataType{ data };

	if (m_data.isAllocated())
	{
		m_status = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	m_status = status;
}

template <class DataType, class AllocatedDataType>
Result<DataType, AllocatedDataType>::Result(DataType&& data, const NTSTATUS status)
{
	m_data = new DataType{ move(data) };

	if (nullptr == m_data)
	{
		m_status = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	m_status = status;
}

template <class DataType, class AllocatedDataType>
Result<DataType, AllocatedDataType>& Result<DataType, AllocatedDataType>::operator=(Result&& other)
{
	m_data.allocate();
	m_status = other.m_status;
	m_data = move(other.m_data);
	return *this;
}

template <class DataType, class AllocatedDataType>
const DataType& Result<DataType, AllocatedDataType>::operator*() const
{
	return *m_data;
}

template <class DataType, class AllocatedDataType>
DataType& Result<DataType, AllocatedDataType>::operator*()
{
	return *m_data;
}

template <class DataType, class AllocatedDataType>
const DataType* Result<DataType, AllocatedDataType>::operator->() const
{
	return m_data.get();
}

template <class DataType, class AllocatedDataType>
DataType* Result<DataType, AllocatedDataType>::operator->()
{
	return m_data.get();
}

template <class DataType, class AllocatedDataType>
NTSTATUS Result<DataType, AllocatedDataType>::getStatus() const
{
	return m_status;
}
