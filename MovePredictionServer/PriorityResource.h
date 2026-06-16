#pragma once



#include "TypeJob.h"
#include "JobQueue.h"
#include "RWLock.h"
#include "ConditionVariableSRW.h"


// 하나의 우선순위 레벨이 소유하는 리소스.

struct PriorityResources
{
	int32_t									_numThreads					= 0;
	std::vector< HANDLE >					_threadHandles;
	std::unique_ptr< JobQueue[] >			_jobQueuePerThread;
	std::atomic< int32_t >					_nextQueue{ 0 };

	ConditionVariableSRW					_sleepingConditionVariable;
	RWLock									_sleepingLock;

	ConditionVariableSRW					_waitingCondition;
	RWLock									_waitingLock;

	PriorityResources( void ) noexcept
		: _waitingCondition( &_waitingLock )
		, _sleepingConditionVariable( &_sleepingLock )
	{
		
	}

	// 내 큐를 우선적으로 확인 한 후에
	// 나머지 할 거 있으면 처리
	inline void								work( int32_t startingQueue ) noexcept
	{
		Job job;
		for ( int32_t ii = 0; ii < _numThreads; ++ii )
		{
			JobQueue& jobQueue				= _jobQueuePerThread[ startingQueue % _numThreads ];
			while ( true == jobQueue.pop( job ) )
			{
				int32_t progressBefore		= job.execute();
				if ( 1 == progressBefore )
				{
					AutoWriteLocker lock( &_waitingLock );
					_waitingCondition.wakeAll();
				}
			}

			startingQueue					+= 1;
		}
	}
};