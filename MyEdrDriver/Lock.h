#pragma once

#include "Concepts.h"

template <typename Type>
concept Lockable = requires(Type type) {
	{ type.acquire() } -> convertible_to<void>;
	{ type.release() } -> convertible_to<void>;
};

template <typename LockableType> requires Lockable<LockableType>
class Lock final
{
public:
	Lock() = delete;
	Lock(const Lock&) = delete;
	Lock(Lock&&) = delete;
	Lock(LockableType& lockable);

	~Lock();

	Lock& operator=(const Lock&) = delete;
	Lock& operator=(Lock&&) = delete;

private:
	LockableType& m_lockable;
};

template<typename LockableType> requires Lockable<LockableType>
Lock<LockableType>::Lock(LockableType& lockable) :
	m_lockable(lockable)
{
	m_lockable.acquire();
}

template<typename LockableType> requires Lockable<LockableType>
Lock<LockableType>::~Lock()
{
	m_lockable.release();
}
