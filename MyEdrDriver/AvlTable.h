#pragma once

#include <ntddk.h>

#include "Memory.h"

template<typename DataType>
class AvlTable final
{
public:
	AvlTable();
	AvlTable(const AvlTable&) = delete;
	AvlTable(AvlTable&&) = delete;

	AvlTable& operator=(const AvlTable&) = delete;
	AvlTable& operator=(AvlTable&&) = delete;

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

	static RTL_GENERIC_COMPARE_RESULTS __stdcall CompareAvlTableEntries(const PRTL_AVL_TABLE table, const AvlTableEntry* first, const AvlTableEntry* second);
	static PVOID __stdcall Allocate(const PRTL_AVL_TABLE table, CLONG byteCount);
	static void __stdcall Free(const PRTL_AVL_TABLE table, PVOID buffer);
};

template<typename DataType>
AvlTable<DataType>::AvlTable() :
	m_table{ nullptr }
{
	RtlInitializeGenericTableAvl(
		&m_table,
		reinterpret_cast<PRTL_AVL_COMPARE_ROUTINE>(CompareAvlTableEntries),
		reinterpret_cast<PRTL_AVL_ALLOCATE_ROUTINE>(Allocate),
		reinterpret_cast<PRTL_AVL_FREE_ROUTINE>(Free),
		nullptr
	);
}

template<typename DataType>
RTL_GENERIC_COMPARE_RESULTS AvlTable<DataType>::CompareAvlTableEntries(const PRTL_AVL_TABLE table, const AvlTableEntry* first, const AvlTableEntry* second)
{
	UNREFERENCED_PARAMETER(table);

	if (first->Data < second->Data)
	{
		return GenericLessThan;
	}

	if (first->Data > second->Data)
	{
		return GenericGreaterThan;
	}

	return GenericEqual;
}

template<typename DataType>
PVOID AvlTable<DataType>::Allocate(const PRTL_AVL_TABLE table, const CLONG byteCount)
{
	UNREFERENCED_PARAMETER(table);

	return DefaultAllocate(byteCount);
}

template<typename DataType>
void AvlTable<DataType>::Free(const PRTL_AVL_TABLE table, PVOID buffer)
{
	UNREFERENCED_PARAMETER(table);

	DefaultDelete(buffer);
}