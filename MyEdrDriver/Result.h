#pragma once

#include "AutoDeletedPointer.h"
#include "Utility.h"

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

template <class DataType>
class Result final
{
public:
	using AllocatedDataType = AutoDeletedPointer<DataType>;

	Result(NTSTATUS status = STATUS_SUCCESS);
	Result(const Result&) = delete;
	Result(Result&&) = default;
	Result(DataType* data, NTSTATUS status = STATUS_SUCCESS);

	Result& operator=(const Result&) = delete;
	Result& operator=(Result&&) = default;

	const DataType& operator*() const;
	DataType& operator*();

	const DataType* operator->() const;
	DataType* operator->();

	NTSTATUS getStatus() const;

	const DataType* getData() const;
	DataType* getData();

	DataType* releaseData();

private:
	NTSTATUS m_status;
	AllocatedDataType m_data;
};

template <class DataType>
Result<DataType>::Result(const NTSTATUS status) :
	m_status{ status }
{
}

template <class DataType>
Result<DataType>::Result(DataType* data, const NTSTATUS status) :
	m_status{ status },
	m_data{ data }
{
}

template <class DataType>
const DataType& Result<DataType>::operator*() const
{
	return *m_data;
}

template <class DataType>
DataType& Result<DataType>::operator*()
{
	return *m_data;
}

template <class DataType>
const DataType* Result<DataType>::operator->() const
{
	return m_data.get();
}

template <class DataType>
DataType* Result<DataType>::operator->()
{
	return m_data.get();
}

template <class DataType>
NTSTATUS Result<DataType>::getStatus() const
{
	return m_status;
}

template <class DataType>
const DataType* Result<DataType>::getData() const
{
	return m_data.get();
}

template <class DataType>
DataType* Result<DataType>::getData()
{
	return m_data.get();
}

template <class DataType>
DataType* Result<DataType>::releaseData()
{
	return m_data.release();
}
