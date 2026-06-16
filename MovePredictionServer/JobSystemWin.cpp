#include "pch.h"
#include "JobSystemWin.h"


InternalState JobSystem::_internalState;


void JobSystem::initialize( int32_t maxThreadCount ) noexcept
{
	if ( 0 < JobSystem::_internalState._numCores )
	{
		return;
	}

	maxThreadCount						= std::max( 1, maxThreadCount );

	JobSystem::_internalState._numCores = std::thread::hardware_concurrency();

	for ( int32_t priority = 0; priority < static_cast< int32_t>( JobPriority::Count ); ++priority )
	{
		const JobPriority jobPriority= (JobPriority)priority;
		PriorityResources& resource	= JobSystem::_internalState._resources[ priority ];

		switch ( jobPriority )
		{
		case JobPriority::High:
			{
				resource._numThreads	= JobSystem::_internalState._numCores - 1;
			}
			break;
		case JobPriority::Low:
			{
				resource._numThreads	= JobSystem::_internalState._numCores - 2;
			}
			break;
		case JobPriority::Streaming:
			{
				resource._numThreads	= 1;
			}
			break;
		default:
			{
				assert( false );
			}
			break;
		}

		resource._numThreads			= std::clamp( resource._numThreads, 1, maxThreadCount );
		resource._jobQueuePerThread.reset( new JobQueueWin[ resource._numThreads ] );
		resource._threadHandles.reserve( resource._numThreads );

		for ( int32_t ii = 0; ii < resource._numThreads; ++ii )
		{
			WorkerThreadParam* param	= new WorkerThreadParam();
			param->_resources			= &resource;
			param->_workerIndex			= ii;
			param->_priority			= jobPriority;
			param->_numCores			= _internalState._numCores;

			unsigned threadId			= 0;
			HANDLE handle				= ( HANDLE )_beginthreadex( nullptr, 0, workerThreadFunc, param, 0, &threadId );
			HDASSERT( 0 != handle, "_beginthreadex 처리에 실패 했습니다." );

			resource._threadHandles.emplace_back( handle );

			int32_t core				= ii + 1;
			if ( JobPriority::Streaming == jobPriority )
			{
				core					= _internalState._numCores - 1 - ii;
			}

			DWORD_PTR affinityMask		= 1ull << core;
			DWORD_PTR affinityResult	= SetThreadAffinityMask( handle, affinityMask );
			assert( 0 < affinityResult );

			if ( JobPriority::High == jobPriority )
			{
				BOOL priorityResult		= SetThreadPriority( handle, THREAD_PRIORITY_NORMAL );
				assert( TRUE == priorityResult );

				std::wstring wthrreadname	= L"JOB THREAD_" + std::to_wstring(  ii );

				HRESULT hr				= SetThreadDescription( handle, wthrreadname.c_str() );
				assert( true == SUCCEEDED( hr ) );
			}
			else if ( JobPriority::Low == jobPriority )
			{
				BOOL priorityResult		= SetThreadPriority( handle, THREAD_PRIORITY_LOWEST );
				assert( TRUE == priorityResult );

				std::wstring wthrreadname	= L"JOB THREAD_LOWER_" + std::to_wstring(  ii );

				HRESULT hr				= SetThreadDescription( handle, wthrreadname.c_str() );
				assert( true == SUCCEEDED( hr ) );
			}
			else if ( JobPriority::Streaming == jobPriority )
			{
				BOOL priorityResult		= SetThreadPriority( handle, THREAD_PRIORITY_BELOW_NORMAL );
				assert( TRUE == priorityResult );

				std::wstring wthrreadname	= L"JOB THREAD_STREAMING_" + std::to_wstring(  ii );

				HRESULT hr				= SetThreadDescription( handle, wthrreadname.c_str() );
				assert( true == SUCCEEDED( hr ) );
			}
		}
	}
}

void JobSystem::shutdown( void ) noexcept
{
	_internalState.shutdown();
}

bool JobSystem::isShuttingDown( void ) noexcept
{
	return false == JobSystem::_internalState._alives.load();
}

int32_t JobSystem::getThreadCount( JobPriority priority ) noexcept
{
	return JobSystem::_internalState._resources[ static_cast< int32_t>( priority )]._numThreads;
}

void JobSystem::execute( JobContext& context, const std::function< void( JobArgs )>& task ) noexcept
{
	PriorityResources& resource		= JobSystem::_internalState._resources[ static_cast< int32_t>( context._priority ) ];

	context._counter.fetch_add( 1 );

	Job job;
	job._context						= &context;
	job._task							= task;
	job._groupId						= 0;
	job._groupJobOffset					= 0;
	job._groupJobEnd					= 1;
	job._sharedMemorySize				= 0;

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
	if ( 0 == jobCount )
	{
		return;
	}

	PriorityResources& resource			= JobSystem::_internalState._resources[ static_cast< int32_t>( context._priority ) ];
	
	const int32_t groupCount			= dispatchGroupCount( jobCount, groupSize );

	context._counter.fetch_add( groupCount );

	Job job;
	job._context						= &context;
	job._task							= task;
	job._sharedMemorySize				= sharedMemorySize;

	for ( int32_t groupId = 0; groupId < groupCount; ++groupId )
	{
		job._groupId					= groupId;
		job._groupJobOffset				= groupId * groupSize;
		job._groupJobEnd				= std::min( job._groupJobOffset + groupSize, jobCount );

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
		PriorityResources& resource		= JobSystem::_internalState._resources[ static_cast< int32_t>( context._priority ) ];
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
