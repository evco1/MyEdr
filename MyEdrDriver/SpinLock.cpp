#include <ntddk.h>

#include "SpinLock.h"
#include "Utility.h"

SpinLock::SpinLock()
{
	KeInitializeSpinLock(&m_spinLock);
}

SpinLock::SpinLock(SpinLock&& other)
{
	*this = move(other);
}

SpinLock& SpinLock::operator=(SpinLock&& other)
{
	if (this != &other)
	{
		m_spinLock = other.m_spinLock;
		KeInitializeSpinLock(&other.m_spinLock);
	}

	return *this;
}

void SpinLock::acquire()
{
	KeAcquireSpinLock(&m_spinLock, &m_oldIrql);
}

void SpinLock::release()
{
	KeReleaseSpinLock(&m_spinLock, m_oldIrql);
}
