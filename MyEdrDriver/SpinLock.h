#pragma once

#include <ntddk.h>

class SpinLock final
{
public:
	SpinLock();
	SpinLock(const SpinLock&) = delete;
	SpinLock(SpinLock&& other);

	~SpinLock() = default;

	SpinLock& operator=(const SpinLock&) = delete;
	SpinLock& operator=(SpinLock&& other);

	void acquire();
	void release();

private:
	KSPIN_LOCK m_spinLock;
	KIRQL m_oldIrql;
};
