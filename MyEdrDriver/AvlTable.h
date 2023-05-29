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

	DataType& findElement(const DataType& data);

	void clear();

private:
#pragma pack(push)
#pragma pack(1)
	struct AvlTableEntry
	{
		RTL_AVL_TABLE AvlTable;
		DataType Data;
	};
#pragma pack(pop)

	RTL_AVL_TABLE m_table;

	static RTL_GENERIC_COMPARE_RESULTS __stdcall compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType* first, const DataType* second);
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
	AutoDeletedPointer<DataType> copiedData = new DataType{ move(data) };
	RETURN_ON_CONDITION(nullptr == copiedData, false);
	*copiedData = move(data);
	RETURN_ON_CONDITION(nullptr == RtlInsertElementGenericTableAvl(&m_table, copiedData.get(), sizeof(DataType), nullptr), false);
	copiedData.release();
	return true;
}

template<typename DataType>
bool AvlTable<DataType>::deleteElement(const DataType& data)
{
	return RtlDeleteElementGenericTableAvl(&m_table, const_cast<PVOID>(&data));
}

template<typename DataType>
DataType& AvlTable<DataType>::findElement(const DataType& data)
{
	return *static_cast<DataType*>(RtlLookupElementGenericTableAvl(&m_table, const_cast<PVOID>(data)));
}

template<typename DataType>
void AvlTable<DataType>::clear()
{
}

template<typename DataType>
RTL_GENERIC_COMPARE_RESULTS AvlTable<DataType>::compareAvlTableEntries(const PRTL_AVL_TABLE table, const DataType* first, const DataType* second)
{
	UNREFERENCED_PARAMETER(table);

	RTL_GENERIC_COMPARE_RESULTS compareResult;

	if (*first < *second)
	{
		compareResult = GenericLessThan;
	}
	else if (*first > *second)
	{
		compareResult = GenericGreaterThan;
	}
	else
	{
		compareResult = GenericEqual;
	}

	DEBUG_PRINT("%p, %p, %p, %u", table, first, second, compareResult);
	return compareResult;
}

template<typename DataType>
PVOID AvlTable<DataType>::allocate(const PRTL_AVL_TABLE table, const CLONG byteCount)
{
	UNREFERENCED_PARAMETER(table);

	PVOID buffer = ::allocate(byteCount);
	DEBUG_PRINT("%p, %u, %p", table, byteCount, buffer);
	return buffer;
}

template<typename DataType>
void AvlTable<DataType>::free(const PRTL_AVL_TABLE table, PVOID buffer)
{
	UNREFERENCED_PARAMETER(table);

	DEBUG_PRINT("%p, %p", table, buffer);
	::free(buffer);
}
