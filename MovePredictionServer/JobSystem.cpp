#include "pch.h"
#include "JobSystem.h"


struct PriorityConfig
{
	int32_t									_threadCountOffset			= 0;
	int32_t									_winPriority				= 0;
	const wchar_t*							_namePrefix					= nullptr;
};

constexpr PriorityConfig					PRIORITY_CONFIGS[ static_cast< int32_t >( JobPriority::COUNT ) ] =
{
	{ 1, THREAD_PRIORITY_NORMAL, L"JOB_THREAD_HIGH_" },
	{ 2, THREAD_PRIORITY_LOWEST, L"JOB_THREAD_LOW_" },
	{ 0, THREAD_PRIORITY_BELOW_NORMAL, L"JOB_THREAD_STREAMING_" },
};

int32_t calculateThreadCount( JobPriority priority, int32_t numCores, int32_t maxThreadCount ) noexcept
{
	HDASSERT( priority != JobPriority::COUNT, "Job Priority값이 비정상 입니다." );
	HDASSERT( 0 < numCores, "Core 값이 비정상 입니다." );
	HDASSERT( 0 < maxThreadCount, "Thread Count 값이 비정상 입니다." );

	int32_t numThreads						= 0;

	if ( JobPriority::STREAMING == priority )
	{
		numThreads							= 1;
	}
	else
	{
		const int32_t offset				= PRIORITY_CONFIGS[ static_cast< int32_t >( priority ) ]._threadCountOffset;
		numThreads							= numCores - offset;
	}

	return std::clamp( numThreads, 1, maxThreadCount );
}

int32_t calculateCoreIndex( JobPriority priority, int32_t workerIndex, int32_t numCores ) noexcept
{
	HDASSERT( priority != JobPriority::COUNT, "Job Priority값이 비정상 입니다." );
	HDASSERT( 0 <= workerIndex, "workerIndex 값이 비정상 입니다." );
	HDASSERT( 0 < numCores, "Core 값이 비정상 입니다." );

	if ( JobPriority::STREAMING == priority )
	{
		return numCores - 1 - workerIndex;
	}

	return workerIndex + 1;
}

void createWorker( JobSystem* jobSystem, PriorityResources& resource, JobPriority priority, int32_t workerIndex, int32_t numCores ) noexcept
{
	HDASSERT( nullptr != jobSystem, "JobSystem이 비정상 입니다." );
	HDASSERT( priority != JobPriority::COUNT, "Job Priority값이 비정상 입니다." );
	HDASSERT( 0 <= workerIndex, "workerIndex 값이 비정상 입니다." );
	HDASSERT( 0 < numCores, "Core 값이 비정상 입니다." );

	WorkerThreadParam* param				= new WorkerThreadParam();
	param->_resources						= &resource;
	param->_workerIndex						= workerIndex;
	param->_priority						= priority;
	param->_numCores						= numCores;
	param->_jobSystem						= jobSystem;

	unsigned threadId						= 0;
	HANDLE handle							= ( HANDLE )::_beginthreadex( nullptr, 0, workerThreadFunc, param, 0, &threadId );
	HDASSERT( 0 != handle, "_beginthreadex 처리에 실패 했습니다." );

	resource._threadHandles.emplace_back( handle );

	const PriorityConfig& config			= PRIORITY_CONFIGS[ static_cast< int32_t >( priority ) ];

	const int32_t core						= calculateCoreIndex( priority, workerIndex, numCores );
	DWORD_PTR affinityResult				= ::SetThreadAffinityMask( handle, 1ull << core );
	HDASSERT( 0 < affinityResult, "Affinity 결과 값이 비정상 입니다." );

	BOOL priorityResult						= ::SetThreadPriority( handle, config._winPriority );
	HDASSERT( true == priorityResult, "Set Thread Priority가 실패 했습니다." );

	std::wstring threadName					= config._namePrefix + std::to_wstring( workerIndex );
	HRESULT hr								= ::SetThreadDescription( handle, threadName.c_str() );
	HDASSERT( true == SUCCEEDED( hr ),  "SetThreadDescription이 실패 했습니다." );
}

JobSystem::~JobSystem( void ) noexcept
{
	_internalState.shutdown( this );
}

void JobSystem::initialize( int32_t maxThreadCount ) noexcept
{
	HDASSERT( 0 <= maxThreadCount, "maxThreadCount 값이 비정상 입니다." );

	if ( 0 < JobSystem::_internalState._numCores )
	{
		return;
	}

	maxThreadCount							= std::max( 1, maxThreadCount );
	const int32_t numCores					= static_cast< int32_t >( std::thread::hardware_concurrency() );
	JobSystem::_internalState._numCores		= numCores;

	for ( int32_t priority = 0; priority < static_cast< int32_t >( JobPriority::COUNT ); ++priority )
	{
		const JobPriority jobPriority		= static_cast< JobPriority >( priority );
		PriorityResources& resource			= JobSystem::_internalState._resources[ priority ];

		resource._numThreads				= calculateThreadCount( jobPriority, numCores, maxThreadCount );
		resource._jobQueuePerThread.reset( new JobQueue[ resource._numThreads ] );
		resource._threadHandles.reserve( resource._numThreads );

		for ( int32_t ii = 0; ii < resource._numThreads; ++ii )
		{
			createWorker( this, resource, jobPriority, ii, numCores );
		}
	}
}

void JobSystem::shutdown( void ) noexcept
{
	_internalState.shutdown( this );
}

bool JobSystem::isShuttingDown( void ) noexcept
{
	return false == JobSystem::_internalState._alives.load();
}

int32_t JobSystem::getThreadCount( JobPriority priority ) noexcept
{
	return JobSystem::_internalState._resources[ static_cast< int32_t >( priority ) ]._numThreads;
}

void JobSystem::execute( JobContext& context, const std::function< void( JobArgs )>& task ) noexcept
{
	PriorityResources& resource				= JobSystem::_internalState._resources[ static_cast< int32_t >( context._priority ) ];

	context._counter.fetch_add( 1 );

	Job job;
	job._context							= &context;
	job._task								= task;
	job._groupId							= 0;
	job._groupJobOffset						= 0;
	job._groupJobEnd						= 1;
	job._sharedMemorySize					= 0;

	if ( resource._numThreads < 1 )
	{
		job.execute();
		return;
	}

	resource._jobQueuePerThread[ resource._nextQueue.fetch_add( 1 ) % resource._numThreads ].push( job );
	resource._sleepingConditionVariable.wakeOne();
}

void JobSystem::dispatch( JobContext& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgs )>& task, int32_t sharedMemorySize ) noexcept
{
	HDASSERT( 0 < jobCount, "JobCount 개수가 비정상 입니다." );
	HDASSERT( 0 < groupSize, "Group Size 정보가 비정상 입니다." );

	if ( 0 == jobCount )
	{
		return;
	}

	PriorityResources& resource				= JobSystem::_internalState._resources[ static_cast< int32_t >( context._priority ) ];
	const int32_t groupCount				= dispatchGroupCount( jobCount, groupSize );

	context._counter.fetch_add( groupCount );

	Job job;
	job._context							= &context;
	job._task								= task;
	job._sharedMemorySize					= sharedMemorySize;

	for ( int32_t groupId = 0; groupId < groupCount; ++groupId )
	{
		job._groupId						= groupId;
		job._groupJobOffset					= groupId * groupSize;
		job._groupJobEnd					= std::min( job._groupJobOffset + groupSize, jobCount );

		if ( resource._numThreads < 1 )
		{
			job.execute();
		}
		else
		{
			resource._jobQueuePerThread[ resource._nextQueue.fetch_add( 1 ) % resource._numThreads ].push( job );
		}
	}

	if ( 1 < resource._numThreads )
	{
		resource._sleepingConditionVariable.wakeAll();
	}
}

int32_t JobSystem::dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept
{
	HDASSERT( 0 < jobCount, "JobCount 개수가 비정상 입니다." );
	HDASSERT( 0 < groupSize, "Group Size 정보가 비정상 입니다." );

	return ( jobCount + groupSize - 1 ) / groupSize;
}

bool JobSystem::isBusy( const JobContext& context ) noexcept
{
	return 0 < context._counter.load();
}

void JobSystem::wait( const JobContext& context ) noexcept
{
	if ( true == isBusy( context ) )
	{
		PriorityResources& resource			= JobSystem::_internalState._resources[ static_cast< int32_t >( context._priority ) ];
		resource._sleepingConditionVariable.wakeAll();
		resource.work( resource._nextQueue.fetch_add( 1 ) % resource._numThreads );

		AutoWriteLocker lock( &resource._waitingLock );

		while ( true == isBusy( context ) )
		{
			resource._waitingCondition.sleepConditionVariable();
		}

	}
}

int32_t JobSystem::getRemainingJobCount( const JobContext& context ) noexcept
{
	return context._counter.load();
}

void InternalState::shutdown( JobSystem* owner ) noexcept
{
	HDASSERT( nullptr != owner, "JobSystem값이 비정상 입니다." );

	if ( true == owner->isShuttingDown() )
	{
		return;
	}

	_alives.store( false );

	bool wakeLoop							= true;

	std::thread waker( [ & ]
	{
		while ( true == wakeLoop )
		{
			for ( PriorityResources& resource : _resources )
			{
				resource._sleepingConditionVariable.wakeAll();
			}
		}
	} );

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

	wakeLoop								= false;
	waker.join();

	for ( PriorityResources& resource : _resources )
	{
		resource._jobQueuePerThread.reset();
		resource._threadHandles.clear();
		resource._numThreads				= 0;
	}

	_numCores								= 0;
}