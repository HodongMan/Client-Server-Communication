#pragma once

#include "RWLock.h"
#include "ConditionVariableSRW.h"


struct InternalState;
struct PriorityResources;


// 스레드 우선순위와 관련이 있음
// Straming은 Main Thread에서 직접 처리
enum class JobPriority
{
	High,
	Low,
	Streaming,
	Count,
};

struct WorkerThreadParam
{
	PriorityResources*			_resources				= nullptr;
	int32_t						_workerIndex			= 0;
	JobPriority					_priority				= JobPriority::Count;
	int32_t						_numCores				= 0;
};


struct JobArgs
{
	int32_t						_jobIndex				= 0;
	int32_t						_groupId				= 0;
	int32_t						_groupIndex				= 0;

	void*						_sharedMemory			= nullptr;
	bool						_isFirstJobInGroup		= false;
	bool						_isLastJobInGroup		= false;
};

struct JobContext
{
	std::atomic< int32_t >		_counter{ 0 };
	JobPriority				_priority				= JobPriority::High;
};

struct Job
{
	std::function< void( JobArgs ) >	_task;
	JobContext*				_context				= nullptr;

	int32_t						_groupId				= 0;
	int32_t						_groupJobOffset			= 0;
	int32_t						_groupJobEnd			= 0;
	int32_t						_sharedMemorySize		= 0;

	inline int32_t				execute( void ) noexcept
	{
		constexpr int32_t		MAX_SHARED_MEMORY		= 4096;

		JobArgs args;
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
	std::deque< Job >		_queue;
	RWLock						_locker;

	inline void					push( const Job& item ) noexcept
	{
		AutoWriteLocker locker( &_locker );

		_queue.push_back( item );
	}

	inline bool					pop( _Out_ Job& item ) noexcept
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

	inline bool					empty( void ) noexcept
	{
		AutoReadLocker locker( &_locker );

		return _queue.empty();
	}
};

struct PriorityResources
{
	int32_t						_numThreads					= 0;
	std::vector< HANDLE >		_threadHandles;
	std::unique_ptr< JobQueueWin[] >	_jobQueuePerThread;
	std::atomic< int32_t >		_nextQueue{ 0 };

	ConditionVariableSRW		_sleepingConditionVariable;
	RWLock						_sleepingLock;

	ConditionVariableSRW		_waitingCondition;
	RWLock						_waitingLock;

	PriorityResources( void ) noexcept
		: _waitingCondition( &_waitingLock )
		, _sleepingConditionVariable( &_sleepingLock )
	{
		
	}

	// 내 큐를 우선적으로 확인 한 후에
	// 나머지 할 거 있으면 처리
	inline void					work( int32_t startingQueue ) noexcept
	{
		Job job;
		for ( int32_t ii = 0; ii < _numThreads; ++ii )
		{
			JobQueueWin& jobQueue	= _jobQueuePerThread[ startingQueue % _numThreads ];
			while ( true == jobQueue.pop( job ) )
			{
				int32_t progressBefore	= job.execute();
				if ( 1 == progressBefore )
				{
					AutoWriteLocker lock( &_waitingLock );
					_waitingCondition.wakeAll();
				}
			}

			startingQueue		+= 1;
		}
	}
};


class JobSystem
{
public:
	static void					initialize( int32_t maxThreadCount = 0 ) noexcept;
	static void					shutdown( void ) noexcept;

	static bool					isShuttingDown( void ) noexcept;

	static int32_t				getThreadCount( JobPriority priority ) noexcept;

	static void					execute( JobContext& context, const std::function< void( JobArgs ) >& task ) noexcept;
	static void					dispatch( JobContext& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgs ) >& task, int32_t sharedMemorySize = 0  ) noexcept;

	static int32_t				dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept;

	static bool					isBusy( const JobContext& context ) noexcept;
	static void					wait( const JobContext& context ) noexcept;

	static int32_t				getRemainingJobCount( const JobContext& context ) noexcept;

	static InternalState		_internalState;
};

struct InternalState
{
	int32_t						_numCores						= 0;
	PriorityResources			_resources[ int32_t( JobPriority::Count ) ];
	std::atomic_bool			_alives{ true };

	void						shutdown( void ) noexcept
	{
		if ( true == JobSystem::isShuttingDown() )
		{
			return;
		}

		_alives.store( false );
		bool wakeLoop			= true;
		std::thread waker( [ & ] 
		{
			while ( true == wakeLoop )
			{
				for ( PriorityResources& resource : _resources )
				{
					resource._sleepingConditionVariable.wakeAll();
				}
			}
		});

		for ( PriorityResources& resource : _resources )
		{
			if ( false == resource._threadHandles.empty() )
			{
				::WaitForMultipleObjects( ( DWORD )resource._threadHandles.size(), resource._threadHandles.data(), TRUE, INFINITE );

				for ( HANDLE handle : resource._threadHandles )
				{
					::CloseHandle( handle );
				}
			}
		}

		wakeLoop				= false;
		waker.join();

		for ( PriorityResources& resource : _resources )
		{
			resource._jobQueuePerThread.reset();
			resource._threadHandles.clear();
			resource._numThreads	= 0;
		}

		_numCores				= 0;
	}

	~InternalState( void ) noexcept
	{
		shutdown();
	}
};

static unsigned __stdcall workerThreadFunc( void* arg ) noexcept
{
	WorkerThreadParam* param	= static_cast< WorkerThreadParam* >( arg );
	PriorityResources& resource	= *param->_resources;

	const int32_t workerIndex	= param->_workerIndex;

	while ( JobSystem::_internalState._alives.load() )
	{
		resource.work( workerIndex );

		AutoWriteLocker lock( &resource._sleepingLock );

		bool hasJob				= false;
		for ( int32_t jj = 0; jj < resource._numThreads; ++jj )
		{
			if ( false == resource._jobQueuePerThread[ jj ].empty() )
			{
				hasJob			= true;
				break;
			}
		}

		if ( false == hasJob )
		{
			resource._sleepingConditionVariable.sleepConditionVariable();
		}
	}

	delete param;
	return 0;
}