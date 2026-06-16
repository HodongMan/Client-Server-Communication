#pragma once


#include "TypeJob.h"
#include "Job.h"
#include "JobQueue.h"
#include "PriorityResource.h"
#include "InternalState.h"
#include "JobWorker.h"


class JobSystem
{
public:
	static void							initialize( int32_t maxThreadCount = 0 ) noexcept;
	static void							shutdown( void ) noexcept;

	static bool							isShuttingDown( void ) noexcept;

	static int32_t						getThreadCount( JobPriority priority ) noexcept;

	static void							execute( JobContext& context, const std::function< void( JobArgs ) >& task ) noexcept;
	static void							dispatch( JobContext& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgs ) >& task, int32_t sharedMemorySize = 0  ) noexcept;

	static int32_t						dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept;

	static bool							isBusy( const JobContext& context ) noexcept;
	static void							wait( const JobContext& context ) noexcept;

	static int32_t						getRemainingJobCount( const JobContext& context ) noexcept;

	static InternalState				_internalState;
};