#include "pch.h"
#include "JobSystemWin.h"


InternalStateWin JobSystemWin::_internalState;


void JobSystemWin::initialize( int32_t maxThreadCount ) noexcept
{
	if ( 0 < JobSystemWin::_internalState._numCores )
	{
		return;
	}

	maxThreadCount						= std::max( 1, maxThreadCount );

	JobSystemWin::_internalState._numCores = std::thread::hardware_concurrency();

	for ( int32_t priority = 0; priority < static_cast< int32_t>( JobPriorityWin::Count ); ++priority )
	{
		const JobPriorityWin jobPriority= (JobPriorityWin)priority;
		PriorityResourcesWin& resource	= JobSystemWin::_internalState._resources[ priority ];

		switch ( jobPriority )
		{
		case JobPriorityWin::High:
			{
				resource._numThreads	= JobSystemWin::_internalState._numCores - 1;
			}
			break;
		case JobPriorityWin::Low:
			{
				resource._numThreads	= JobSystemWin::_internalState._numCores - 2;
			}
			break;
		case JobPriorityWin::Streaming:
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
		resource._threads.reserve( resource._numThreads );

		for ( int32_t ii = 0; ii < resource._numThreads; ++ii )
		{
			std::thread& worker			= resource._threads.emplace_back([ ii, priority, &resource ] 
			{
				while ( true == JobSystemWin::_internalState._alives.load() )
				{
					resource.work( ii );

					AutoWriteLocker lock( &resource._sleepingMutex );
					resource._sleepingConditionVariable.sleepConditionVariable();
				}
			});

			std::thread::native_handle_type handle = worker.native_handle();
			int32_t core				= ii + 1;
			if ( JobPriorityWin::Streaming == jobPriority )
			{
				core					= JobSystemWin::_internalState._numCores - 1 - ii;
			}

			DWORD_PTR affinityMask		= 1ull << core;
			DWORD_PTR affinityResult	= SetThreadAffinityMask( handle, affinityMask );
			assert( 0 < affinityResult );

			if ( JobPriorityWin::High == jobPriority )
			{
				BOOL priorityResult		= SetThreadPriority( handle, THREAD_PRIORITY_NORMAL );
				assert( TRUE == priorityResult );

				std::wstring wthrreadname	= L"JOB THREAD_" + std::to_wstring(  ii );

				HRESULT hr				= SetThreadDescription( handle, wthrreadname.c_str() );
				assert( true == SUCCEEDED( hr ) );
			}
			else if ( JobPriorityWin::Low == jobPriority )
			{
				BOOL priorityResult		= SetThreadPriority( handle, THREAD_PRIORITY_LOWEST );
				assert( TRUE == priorityResult );

				std::wstring wthrreadname	= L"JOB THREAD_LOWER_" + std::to_wstring(  ii );

				HRESULT hr				= SetThreadDescription( handle, wthrreadname.c_str() );
				assert( true == SUCCEEDED( hr ) );
			}
			else if ( JobPriorityWin::Streaming == jobPriority )
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

void JobSystemWin::shutdown(void) noexcept
{
}

bool JobSystemWin::isShuttingDown(void) noexcept
{
	return false == JobSystemWin::_internalState._alives.load();
}

int32_t JobSystemWin::getThreadCount( JobPriorityWin priority ) noexcept
{
	return JobSystemWin::_internalState._resources[ static_cast< int32_t>( priority )]._numThreads;
}

void JobSystemWin::execute( JobContextWin& context, const std::function< void( JobArgsWin )>& task ) noexcept
{
	PriorityResourcesWin& resource		= JobSystemWin::_internalState._resources[ static_cast< int32_t>( context._priority ) ];

	context._counter.fetch_add( 1 );

	JobWin job;
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

void JobSystemWin::dispatch( JobContextWin& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgsWin )>& task, int32_t sharedMemorySize ) noexcept
{
	if ( 0 == jobCount )
	{
		return;
	}

	PriorityResourcesWin& resource		= JobSystemWin::_internalState._resources[ static_cast< int32_t>( context._priority ) ];
	
	const int32_t groupCount			= dispatchGroupCount( jobCount, groupSize );

	context._counter.fetch_add( groupCount );

	JobWin job;
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

int32_t JobSystemWin::dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept
{
	return ( jobCount + groupSize - 1 ) / groupSize;
}

bool JobSystemWin::isBusy( const JobContextWin& context ) noexcept
{
	return 0 < context._counter.load();
}

void JobSystemWin::wait( const JobContextWin& context ) noexcept
{
	if ( true == isBusy( context ) )
	{
		PriorityResourcesWin& resource	= JobSystemWin::_internalState._resources[ static_cast< int32_t>( context._priority ) ];
		resource._sleepingConditionVariable.wakeAll();
		resource.work( resource._nextQueue.fetch_add( 1 ) % resource._numThreads );

		AutoWriteLocker lock( &resource._waitingMutex );

		while ( true == isBusy( context ) )
		{
			resource._waitingCondition.sleepConditionVariable();
		}
	}
}

int32_t JobSystemWin::getRemainingJobCount( const JobContextWin& context ) noexcept
{
	return context._counter.load();
}
