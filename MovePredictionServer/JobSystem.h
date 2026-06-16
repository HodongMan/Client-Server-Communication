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
	JobSystem( void ) noexcept = default;
	~JobSystem( void ) noexcept;

	void							initialize( int32_t maxThreadCount = 0 ) noexcept;
	void							shutdown( void ) noexcept;

	bool							isShuttingDown( void ) noexcept;

	int32_t							getThreadCount( JobPriority priority ) noexcept;

	void							execute( JobContext& context, const std::function< void( JobArgs ) >& task ) noexcept;
	void							dispatch( JobContext& context, int32_t jobCount, int32_t groupSize, const std::function< void( JobArgs ) >& task, int32_t sharedMemorySize = 0  ) noexcept;

	int32_t							dispatchGroupCount( int32_t jobCount, int32_t groupSize ) noexcept;

	bool							isBusy( const JobContext& context ) noexcept;
	void							wait( const JobContext& context ) noexcept;

	int32_t							getRemainingJobCount( const JobContext& context ) noexcept;

public:
	InternalState					_internalState;
};