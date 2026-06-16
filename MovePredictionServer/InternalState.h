#pragma once


#include "TypeJob.h"
#include "PriorityResource.h"

class JobSystem;


struct InternalState
{
	int32_t								_numCores						= 0;
	PriorityResources					_resources[ int32_t( JobPriority::COUNT ) ];
	std::atomic_bool					_alives{ true };

	void								shutdown( JobSystem* owner ) noexcept;
};