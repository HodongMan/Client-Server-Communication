#pragma once


#include "TypeJob.h"
#include "PriorityResource.h"

struct InternalState
{
	int32_t								_numCores						= 0;
	PriorityResources					_resources[ int32_t( JobPriority::COUNT ) ];
	std::atomic_bool					_alives{ true };

	~InternalState( void ) noexcept
	{
		shutdown();
	}

	void								shutdown( void ) noexcept;
};