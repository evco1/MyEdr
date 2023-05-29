#pragma once

#include <ntddk.h>

#include "Memory.h"
#include "AutoDeletedPointer.h"

template<typename DataType>
class AvlTable final
{
public:
	AvlTable();
	AvlTable(const AvlTable&) = delete;
	AvlTable(AvlTable&&) = delete;

	~AvlTable();

	AvlTable& operator=(const AvlTable&) = delete;
	AvlTable& operator=(AvlTable&&) = delete;

	bool insertElement(const DataType& data);
	bool insertElement(DataType&& data);

	bool deleteElement(const DataType& data);

	DataType* findElement(const DataType& data) const;

	bool containsElement(const DataType& data) const;

	void clear();

private:
	RTL_AVL_TABLE m_table;

	static RTL_GENERIC_COMPARE_RESULTS __stdcall compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType** first, const DataType** second);
	static PVOID __stdcall allocate(const PRTL_AVL_TABLE table, CLONG byteCount);
	static void __stdcall free(const PRTL_AVL_TABLE table, PVOID buffer);
};

template<typename DataType>
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

template<typename DataType>
AvlTable<DataType>::~AvlTable()
{
	clear();
}

template<typename DataType>
bool AvlTable<DataType>::insertElement(const DataType& data)
{
	DataType copiedData = data;
	return insertElement(move(copiedData));
}

template<typename DataType>
bool AvlTable<DataType>::insertElement(DataType&& data)
{
	AutoDeletedPointer<DataType> movedData = new DataType{ move(data) };
	RETURN_ON_CONDITION(nullptr == movedData, false);
	RETURN_ON_CONDITION(nullptr == RtlInsertElementGenericTableAvl(&m_table, &movedData.get(), sizeof(DataType*), nullptr), false);
	movedData.release();
	return true;
}

template<typename DataType>
bool AvlTable<DataType>::deleteElement(const DataType& data)
{
	AutoDeletedPointer<DataType> foundData = findElement(data);
	RETURN_ON_CONDITION(nullptr == foundData, false);
	DEBUG_PRINT("%u, %u", data, *foundData);
	return RtlDeleteElementGenericTableAvl(&m_table, &foundData);
}

template<typename DataType>
DataType* AvlTable<DataType>::findElement(const DataType& data) const
{
	const DataType* pData = &data;
	DataType** foundData = static_cast<DataType**>(RtlLookupElementGenericTableAvl(
		const_cast<PRTL_AVL_TABLE>(&m_table),
		const_cast<DataType**>(&pData)
	));
	RETURN_ON_CONDITION(nullptr == foundData, nullptr);
	return *foundData;
}

template<typename DataType>
bool AvlTable<DataType>::containsElement(const DataType& data) const
{
	return nullptr != findElement(data);
}

template<typename DataType>
void AvlTable<DataType>::clear()
{
	DataType** data = static_cast<DataType**>(RtlEnumerateGenericTableAvl(&m_table, TRUE));

	while (nullptr != data) {
		deleteElement(**data);
		data = static_cast<DataType**>(RtlEnumerateGenericTableAvl(&m_table, TRUE));
	}
}

template<typename DataType>
RTL_GENERIC_COMPARE_RESULTS AvlTable<DataType>::compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType** first, const DataType** second)
{
	UNREFERENCED_PARAMETER(table);

	DEBUG_PRINT("%u, %u", **first, **second);

	RTL_GENERIC_COMPARE_RESULTS compareResult;

	if (**first < **second)
	{
		compareResult = GenericLessThan;
	}
	else if (**first > **second)
	{
		compareResult = GenericGreaterThan;
	}
	else
	{
		compareResult = GenericEqual;
	}

	return compareResult;
}

template<typename DataType>
PVOID AvlTable<DataType>::allocate(const PRTL_AVL_TABLE table, const CLONG byteCount)
{
	UNREFERENCED_PARAMETER(table);

	PVOID buffer = ::allocate(byteCount);
	return buffer;
}

template<typename DataType>
void AvlTable<DataType>::free(const PRTL_AVL_TABLE table, PVOID buffer)
{
	UNREFERENCED_PARAMETER(table);

	DEBUG_PRINT("%u", **reinterpret_cast<ULONG**>(static_cast<PRTL_BALANCED_LINKS>(buffer) + 1));

	::free(buffer);
}
