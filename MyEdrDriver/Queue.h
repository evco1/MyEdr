#pragma once

#include "AutoDeletedPointer.h"
#include "Result.h"
#include "move.h"
#include "Statuses.h"

#include <wdm.h>

const UINT32 MY_EDR_QUEUE_ENTRIES_TAG = 'eqem';
const size_t DEFAULT_QUEUE_MAX_ENTRY_COUNT = 1000;

template<typename DataType>
class Queue final
{
public:
	Queue() = delete;
	Queue(size_t maxEntryCount = DEFAULT_QUEUE_MAX_ENTRY_COUNT);
	Queue(const Queue&) = delete;
	Queue(Queue&& other);

	~Queue();

	Queue& operator=(const Queue&) = delete;
	Queue& operator=(Queue&& other);

	bool isEmpty() const;
	bool isFull() const;

	Result<DataType> pushTail(const DataType& data);
	Result<DataType> pushTail(DataType&& data);

	Result<DataType> popHead();

	NTSTATUS clear();

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
	size_t m_maxEntryCount;
	size_t m_currentEntryCount;
};

template<typename DataType>
Queue<DataType>::Queue(const size_t maxEntryCount) :
	m_maxEntryCount{ maxEntryCount },
	m_currentEntryCount{ 0 }
{
	InitializeListHead(&m_head);
}

template<typename DataType>
Queue<DataType>::Queue(Queue&& other)
{
	*this = move(other);
}

template<typename DataType>
Queue<DataType>::~Queue()
{
	clear();
}

template<typename DataType>
Queue<DataType>& Queue<DataType>::operator=(Queue&& other)
{
	if (this != &other)
	{
		clear();
		m_maxEntryCount = other.m_maxEntryCount;
		m_currentEntryCount = other.m_currentEntryCount;
		m_head = other.m_head;
		InitializeListHead(&other.m_head);
		other.m_currentEntryCount = 0;
	}

	return *this;
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
	RETURN_ON_CONDITION(isFull(), STATUS_MY_EDR_QUEUE_IS_FULL);

	QueueEntryPointer queueEntry;
	RETURN_STATUS_ON_BAD_STATUS(queueEntry.allocate());

	queueEntry->Data = move(data);
	InsertTailList(&m_head, &queueEntry->ListEntry);
	++m_currentEntryCount;

	//// Release the queue entry so it will no be deallocated at the end of this scope
	queueEntry.release();
	return { STATUS_SUCCESS };
}

template<typename DataType>
Result<DataType> Queue<DataType>::popHead()
{
	RETURN_ON_CONDITION(isEmpty(), STATUS_MY_EDR_QUEUE_IS_EMPTY);

	QueueEntryPointer queueEntry = reinterpret_cast<QueueEntry*>(RemoveHeadList(&m_head));
	--m_currentEntryCount;

	return move(queueEntry->Data);
}

template<typename DataType>
NTSTATUS Queue<DataType>::clear()
{
	NTSTATUS status;

	do {
		status = popHead().getStatus();
	} while (status == STATUS_SUCCESS);

	RETURN_ON_CONDITION(STATUS_MY_EDR_QUEUE_IS_EMPTY == status, STATUS_SUCCESS);
	return status;
}
