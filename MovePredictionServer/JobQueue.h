#pragma once


#include "Job.h"
#include "RWLock.h"


// 내부 lock
struct JobQueue
{
	std::deque< Job >					_queue;
	RWLock								_locker				= {};

	inline void							push( const Job& item ) noexcept
	{
		AutoWriteLocker locker( &_locker );

		_queue.push_back( item );
	}

	inline bool							pop( _Out_ Job& item ) noexcept
	{
		AutoWriteLocker locker( &_locker );

		if ( true == _queue.empty() )
		{
			return false;
		}

		item							= std::move( _queue.front() );
		_queue.pop_front();

		return true;
	}

	inline bool							empty( void ) noexcept
	{
		AutoReadLocker locker( &_locker );

		return _queue.empty();
	}
};