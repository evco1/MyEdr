#include <ntddk.h>

#include "Mutex.h"
#include "Utility.h"

Mutex::Mutex()
{
	ExInitializeFastMutex(&m_mutex);
}

Mutex::Mutex(Mutex&& other)
{
	*this = move(other);
}

Mutex& Mutex::operator=(Mutex&& other)
{
	if (this != &other)
	{
		m_mutex = other.m_mutex;
		ExInitializeFastMutex(&other.m_mutex);
	}

	return *this;
}

void Mutex::acquire()
{
	ExAcquireFastMutex(&m_mutex);
}

void Mutex::release()
{
	ExReleaseFastMutex(&m_mutex);
}
