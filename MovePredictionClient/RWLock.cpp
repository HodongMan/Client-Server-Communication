#include "pch.h"
#include "RWLock.h"


RWLock::RWLock( void ) noexcept
{
	::InitializeSRWLock( &_locker );
}

void RWLock::readLock( void ) noexcept
{
	::AcquireSRWLockShared( &_locker );
}

void RWLock::writeLock( void ) noexcept
{
	::AcquireSRWLockExclusive( &_locker );
}

void RWLock::readUnlock( void ) noexcept
{
	::ReleaseSRWLockShared( &_locker );
}

void RWLock::writeUnlock( void ) noexcept
{
	::ReleaseSRWLockExclusive( &_locker );
}

AutoReadLocker::AutoReadLocker( RWLock* locker ) noexcept
	: _locker{ locker }
{
	assert( nullptr != locker );
	_locker->readLock();
}

AutoReadLocker::~AutoReadLocker( void ) noexcept
{
	assert( nullptr != _locker );
	_locker->readUnlock();
}

AutoWriteLocker::AutoWriteLocker( RWLock* locker ) noexcept
	: _locker{ locker }
{
	assert(nullptr != locker);
	_locker->writeLock();
}

AutoWriteLocker::~AutoWriteLocker( void ) noexcept
{
	assert(nullptr != _locker);
	_locker->writeUnlock();
}
