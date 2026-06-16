#include "pch.h"
#include "JobWorker.h"

#include "PriorityResource.h"
#include "JobSystem.h"


unsigned __stdcall workerThreadFunc( void* arg ) noexcept
{
	WorkerThreadParam* param			= static_cast< WorkerThreadParam* >( arg );
	PriorityResources& resource			= *param->_resources;

	const int32_t workerIndex			= param->_workerIndex;

	while ( JobSystem::_internalState._alives.load() )
	{
		resource.work( workerIndex );

		AutoWriteLocker lock( &resource._sleepingLock );

		bool hasJob						= false;
		for ( int32_t jj = 0; jj < resource._numThreads; ++jj )
		{
			if ( false == resource._jobQueuePerThread[ jj ].empty() )
			{
				hasJob					= true;
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