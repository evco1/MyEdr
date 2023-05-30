#pragma once

#include <ntddk.h>

#include "Memory.h"
#include "Statuses.h"

template <class DataType>
class AvlTable final
{
public:
	AvlTable();
	AvlTable(const AvlTable&) = delete;
	AvlTable(AvlTable&&) = delete;

	~AvlTable();

	AvlTable& operator=(const AvlTable&) = delete;
	AvlTable& operator=(AvlTable&&) = delete;

	NTSTATUS insertElement(DataType* data);

	Result<DataType> deleteElement(const DataType* data);

	const DataType* findElement(const DataType* data) const;
	DataType* findElement(const DataType* data);

	bool containsElement(const DataType* data) const;

	void clear();

private:
	RTL_AVL_TABLE m_table;

	static RTL_GENERIC_COMPARE_RESULTS __stdcall compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType** first, const DataType** second);
	static PVOID __stdcall allocate(const PRTL_AVL_TABLE table, CLONG byteCount);
	static void __stdcall free(const PRTL_AVL_TABLE table, PVOID buffer);
};

template <class DataType>
AvlTable<DataType>::AvlTable() :
	m_table{ nullptr }
{
	RtlInitializeGenericTableAvl(
		&m_table,
		reinterpret_cast<PRTL_AVL_COMPARE_ROUTINE>(compareAvlTableEntries),
		reinterpret_cast<PRTL_AVL_ALLOCATE_ROUTINE>(allocate),
		reinterpret_cast<PRTL_AVL_FREE_ROUTINE>(free),
		nullptr
	);
}

template <class DataType>
AvlTable<DataType>::~AvlTable()
{
	clear();
}

template <class DataType>
NTSTATUS AvlTable<DataType>::insertElement(DataType* data)
{
	RETURN_ON_CONDITION(nullptr == data, false);
	RETURN_ON_CONDITION(
		nullptr == RtlInsertElementGenericTableAvl(&m_table, &data, sizeof(DataType*), nullptr),
		{ STATUS_MY_EDR_AVL_TABLE_INSERT_ELEMENT_FAILED }
	);
	
	return STATUS_SUCCESS;
}

template <class DataType>
Result<DataType> AvlTable<DataType>::deleteElement(const DataType* data)
{
	DataType* foundData = findElement(data);
	RETURN_ON_CONDITION(nullptr == foundData, { STATUS_MY_EDR_AVL_TABLE_ELEMENT_WAS_NOT_FOUND });
	RETURN_ON_CONDITION(
		!RtlDeleteElementGenericTableAvl(&m_table, &data),
		{ STATUS_MY_EDR_AVL_TABLE_DELETE_ELEMENT_FAILED }
	);
	return foundData;
}

template <class DataType>
const DataType* AvlTable<DataType>::findElement(const DataType* data) const
{
	DataType** foundData = static_cast<DataType**>(RtlLookupElementGenericTableAvl(
		const_cast<PRTL_AVL_TABLE>(&m_table),
		const_cast<DataType**>(&data)
	));
	RETURN_ON_CONDITION(nullptr == foundData, nullptr);
	return *foundData;
}

template <class DataType>
DataType* AvlTable<DataType>::findElement(const DataType* data)
{
	DataType** foundData = static_cast<DataType**>(RtlLookupElementGenericTableAvl(
		const_cast<PRTL_AVL_TABLE>(&m_table),
		const_cast<DataType**>(&data)
	));
	RETURN_ON_CONDITION(nullptr == foundData, nullptr);
	return *foundData;
}

template <class DataType>
bool AvlTable<DataType>::containsElement(const DataType* data) const
{
	return nullptr != findElement(data);
}

template <class DataType>
void AvlTable<DataType>::clear()
{
	DataType** data = static_cast<DataType**>(RtlEnumerateGenericTableAvl(&m_table, TRUE));

	while (nullptr != data) {
		deleteElement(*data);
		data = static_cast<DataType**>(RtlEnumerateGenericTableAvl(&m_table, TRUE));
	}
}

template <class DataType>
RTL_GENERIC_COMPARE_RESULTS AvlTable<DataType>::compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType** first, const DataType** second)
{
	UNREFERENCED_PARAMETER(table);

	if (**first < **second)
	{
		return GenericLessThan;
	}
	
	if (**first > **second)
	{
		return GenericGreaterThan;
	}

	return GenericEqual;
}

template <class DataType>
PVOID AvlTable<DataType>::allocate(const PRTL_AVL_TABLE table, const CLONG byteCount)
{
	UNREFERENCED_PARAMETER(table);

	PVOID buffer = ::allocate(byteCount);
	return buffer;
}

template <class DataType>
void AvlTable<DataType>::free(const PRTL_AVL_TABLE table, PVOID buffer)
{
	UNREFERENCED_PARAMETER(table);

	::free(buffer);
}
