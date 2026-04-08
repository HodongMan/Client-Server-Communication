#pragma once



class RWLock
{
public:
	RWLock( void ) noexcept;

	void					readLock( void ) noexcept;
	void					writeLock( void ) noexcept;

	void					readUnlock( void ) noexcept;
	void					writeUnlock( void ) noexcept;

public:
	SRWLOCK					_locker;
};


class AutoReadLocker
{
public:
	AutoReadLocker( RWLock* locker ) noexcept;
	~AutoReadLocker( void ) noexcept;

private:
	RWLock*					_locker;
};

class AutoWriteLocker
{
public:
	AutoWriteLocker( RWLock* locker ) noexcept;
	~AutoWriteLocker( void ) noexcept;

private:
	RWLock*					_locker;
};