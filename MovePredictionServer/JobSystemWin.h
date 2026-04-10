#pragma once

#include "RWLock.h"
#include "ConditionVariableSRW.h"


struct InternalStateWin;


// 스레드 우선순위와 관련이 있음
// Straming은 Main Thread에서 직접 처리
enum class JobPriorityWin
{
	High,
	Low,
	Streaming,
	Count,
};


struct JobArgsWin
{
	int32_t						_jobIndex				= 0;
	int32_t						_groupId				= 0;
	int32_t						_groupIndex				= 0;

	void*						_sharedMemory			= nullptr;
	bool						_isFirstJobInGroup		= false;
	bool						_isLastJobInGroup		= false;
};

struct JobContextWin
{
	std::atomic< int32_t >		_counter{ 0 };
	JobPriorityWin				_priority				= JobPriorityWin::High;
};

struct JobWin
{
	std::function< void( JobArgsWin ) >	_task;
	JobContextWin*				_context				= nullptr;

	int32_t						_groupId				= 0;
	int32_t						_groupJobOffset			= 0;
	int32_t						_groupJobEnd			= 0;
	int32_t						_sharedMemorySize		= 0;

	inline int32_t				execute( void ) noexcept
	{
		constexpr int32_t		MAX_SHARED_MEMORY		= 4096;

		JobArgsWin args;
		args._groupId			= _groupId;
		if ( 0 < _sharedMemorySize && _sharedMemorySize < MAX_SHARED_MEMORY )
		{
			args._sharedMemory	= alloca( _sharedMemorySize );
		}
		else
		{
			args._sharedMemory	= nullptr;
		}

		for ( int32_t ii = _groupJobOffset; ii < _groupJobEnd; ++ii )
		{
			args._jobIndex		= ii;
			args._groupIndex	= ii - _groupJobOffset;
			args._isFirstJobInGroup		= ( ii == _groupJobOffset );
			args._isLastJobInGroup		= ( ii == _groupJobEnd - 1 );

			_task( args );
		}

		/*
		HANDLE threadHandle		= ::GetCurrentThread();
		assert( INVALID_HANDLE_VALUE != threadHandle );

		PWSTR value;
		HRESULT hr = GetThreadDescription( threadHandle, &value );
		if ( true == SUCCEEDED( hr ) )
		{   
			printf( "%ls\n", value );
			LocalFree( value );
		}
		*/
		return _context->_counter.fetch_sub( 1 );
	}
};

struct JobQueueWin
{
	std::deque< JobWin >		_queue;
	RWLock						_locker;

	inline void					push( const JobWin& item ) noexcept
	{
		AutoWriteLocker locker( &_locker );

		_queue.push_back( item );
	}

	inline bool					pop( _Out_ JobWin& item ) noexcept
	{
		AutoWriteLocker locker( &_locker );

		if ( true == _queue.empty() )
		{
			return false;
		}

		item					= std::move( _queue.front() );
		_queue.pop_front();

		return true;
	}
};

struct PriorityResourcesWin
{
	int32_t						_numThreads					= 0;
	std::vector< std::thread >	_threads;
	std::unique_ptr< JobQueueWin[] >	_jobQueuePerThread;
	std::atomic< int32_t >		_nextQueue{ 0 };

	ConditionVariableSRW		_sleepingConditionVariable;
	RWLock						_sleepingMutex;

	ConditionVariableSRW		_waitingCondition;
	RWLock						_waitingMutex;

	PriorityResourcesWin( void ) noexcept
		: _waitingCondition( &_waitingMutex )
		, _sleepingConditionVariable( &_sleepingMutex )
	{
		
	}

	// 내 큐를 우선적으로 확인 한 후에
	// 나머지 할 거 있으면 처리
	inline void					work( int32_t startingQueue ) noexcept
	{
		JobWin job;
		for ( int32_t ii = 0; ii < _numThreads; ++ii )
		{
			JobQueueWin& jobQueue	= _jobQueuePerThread[ startingQueue % _numThreads ];
			while ( true == jobQueue.pop( job ) )
			{
				int32_t progressBefore	= job.execute();
				if ( 1 == progressBefore )
				{
					AutoWriteLocker lock( &_waitingMutex );
					_waitingCondition.wakeAll();
				}
			}

			startingQueue		+= 1;
		}
	}
};


class JobSystemWin
{
public:
	static void					initialize( int32_t maxThreadCount = 0 ) noexcept;
	static void					shutdown( void ) noexcept;

	static bool					isShuttingDown( void ) noexcept;

	static int32_t				getThreadCount( JobPriorityWin priority ) noexcept;

	static void					execute( JobContextWin& context, const std::function< void( JobArgsWin ) >& task ) noexcept;
	static void					dispatch( JobContextWin& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgsWin ) >& task, int32_t sharedMemorySize = 0  ) noexcept;

	static int32_t				dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept;

	static bool					isBusy( const JobContextWin& context ) noexcept;
	static void					wait( const JobContextWin& context ) noexcept;

	static int32_t				getRemainingJobCount( const JobContextWin& context ) noexcept;

	static InternalStateWin		_internalState;
};

struct InternalStateWin
{
	int32_t						_numCores						= 0;
	PriorityResourcesWin		_resources[ int32_t( JobPriorityWin::Count ) ];
	std::atomic_bool			_alives{ true };

	void						shutdown( void ) noexcept
	{
		if ( true == JobSystemWin::isShuttingDown() )
		{
			return;
		}

		_alives.store( false );
		bool wakeLoop			= true;
		std::thread waker( [ & ] 
		{
			while ( true == wakeLoop )
			{
				for ( PriorityResourcesWin& resource : _resources )
				{
					resource._sleepingConditionVariable.wakeAll();
				}
			}
		});

		for (PriorityResourcesWin& resource : _resources )
		{
			for ( std::thread& thread : resource._threads )
			{
				thread.join();
			}
		}

		wakeLoop				= false;
		waker.join();

		for (PriorityResourcesWin& resource : _resources )
		{
			resource._jobQueuePerThread.reset();
			resource._threads.clear();
			resource._numThreads	= 0;
		}

		_numCores				= 0;
	}

	~InternalStateWin( void ) noexcept
	{
		shutdown();
	}
};