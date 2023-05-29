#pragma once

#include <ntddk.h>

#include "Memory.h"

const UINT32 MY_EDR_AVL_TABLE_ENTRIES_TAG = 'eaem';

template<typename DataType, UINT32 Tag = MY_EDR_AVL_TABLE_ENTRIES_TAG>
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

	static RTL_GENERIC_COMPARE_RESULTS __stdcall compareAvlTableEntries(const PRTL_AVL_TABLE table, const AvlTableEntry* first, const AvlTableEntry* second);
	static PVOID __stdcall allocate(const PRTL_AVL_TABLE table, CLONG byteCount);
	static void __stdcall free(const PRTL_AVL_TABLE table, PVOID buffer);
};

template<typename DataType, UINT32 Tag>
AvlTable<DataType, Tag>::AvlTable() :
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

template<typename DataType, UINT32 Tag>
RTL_GENERIC_COMPARE_RESULTS AvlTable<DataType, Tag>::compareAvlTableEntries(const PRTL_AVL_TABLE table, const AvlTableEntry* first, const AvlTableEntry* second)
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

template<typename DataType, UINT32 Tag>
PVOID AvlTable<DataType, Tag>::allocate(const PRTL_AVL_TABLE table, const CLONG byteCount)
{
	UNREFERENCED_PARAMETER(table);

	return ::allocate(byteCount);
}

template<typename DataType, UINT32 Tag>
void AvlTable<DataType, Tag>::free(const PRTL_AVL_TABLE table, PVOID buffer)
{
	UNREFERENCED_PARAMETER(table);

	::free(buffer);
}
