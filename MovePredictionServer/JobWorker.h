#pragma once


#include "TypeJob.h"


struct PriorityResources;
class JobSystem;


struct WorkerThreadParam
{
	JobSystem*							_jobSystem				= nullptr;
	PriorityResources*					_resources				= nullptr;
	int32_t								_workerIndex			= 0;
	JobPriority							_priority				= JobPriority::COUNT;
	int32_t								_numCores				= 0;
};


unsigned __stdcall						workerThreadFunc( void* arg ) noexcept;