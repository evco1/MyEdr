#pragma once

#include <ntddk.h>

class Mutex final
{
public:
	Mutex();
	Mutex(const Mutex&) = delete;
	Mutex(Mutex&& other);

	~Mutex() = default;

	Mutex& operator=(const Mutex&) = delete;
	Mutex& operator=(Mutex&& other);

	void acquire();
	void release();

private:
	FAST_MUTEX m_mutex;
};
