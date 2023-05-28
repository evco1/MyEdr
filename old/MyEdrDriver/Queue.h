#pragma once

#include "AutoDeletedPointer.h"
#include "Result.h"
#include "move.h"
#include "Statuses.h"
#include "Debug.h"

#include <wdm.h>

const UINT32 MY_EDR_QUEUE_ENTRIES_TAG = 'eqem';

template<typename DataType>
class Queue final
{
public:
	constexpr Queue(size_t maxEntryCount);
	Queue(const Queue&) = delete;
	Queue(Queue&&) = default;

	~Queue();

	Queue& operator=(const Queue&) = delete;
	Queue& operator=(Queue&&) = default;

	bool isEmpty() const;
	bool isFull() const;

	Result<DataType> pushTail(const DataType& data);
	Result<DataType> pushTail(DataType&& data);

	Result<DataType> popHead();

	Result<> clear();

private:
#pragma pack(push)
#pragma pack(1)
	struct QueueEntry
	{
		LIST_ENTRY ListEntry;
		DataType Data;
	};
#pragma pack(pop)

	using QueueEntryPointer = AutoDeletedPointer<QueueEntry, MY_EDR_QUEUE_ENTRIES_TAG>;

	LIST_ENTRY m_head;
	const size_t m_maxEntryCount;
	size_t m_currentEntryCount;
};

template<typename DataType>
constexpr Queue<DataType>::Queue(const size_t maxEntryCount) :
	m_maxEntryCount{ maxEntryCount },
	m_currentEntryCount{}
{
	InitializeListHead(&m_head);
}

template<typename DataType>
Queue<DataType>::~Queue()
{
	clear();
}

template<typename DataType>
bool Queue<DataType>::isEmpty() const
{
	return m_currentEntryCount == 0;
}

template<typename DataType>
bool Queue<DataType>::isFull() const
{
	return m_currentEntryCount == m_maxEntryCount;
}

template<typename DataType>
Result<DataType> Queue<DataType>::pushTail(const DataType& data)
{
	DataType copiedData = data;
	return pushTail(move(copiedData));
}

template<typename DataType>
Result<DataType> Queue<DataType>::pushTail(DataType&& data)
{
	if (isFull())
	{
		return NTSTATUS(STATUS_MY_EDR_QUEUE_IS_FULL);
	}

	QueueEntryPointer queueEntry;

	if (!queueEntry.isAllocated())
	{
		return NTSTATUS(STATUS_INSUFFICIENT_RESOURCES);
	}

	InsertTailList(&m_head, &queueEntry->ListEntry);
	++m_currentEntryCount;

	// Release the queue entry so it will no be deallocated in the end of this scope
	queueEntry.release();

	queueEntry->Data = move(data);
	return NTSTATUS{STATUS_SUCCESS};
}

template<typename DataType>
Result<DataType> Queue<DataType>::popHead()
{
	if (isEmpty())
	{
		return NTSTATUS(STATUS_MY_EDR_QUEUE_IS_EMPTY);
	}

	QueueEntryPointer queueEntry = reinterpret_cast<QueueEntry*>(RemoveHeadList(&m_head));
	--m_currentEntryCount;

	return move(queueEntry->Data);
}

template<typename DataType>
Result<> Queue<DataType>::clear()
{
	NTSTATUS status;

	do {
		status = popHead().getStatus();
	} while (status == STATUS_SUCCESS);

	return NTSTATUS(status == STATUS_MY_EDR_QUEUE_IS_EMPTY ? STATUS_SUCCESS : status);
}
