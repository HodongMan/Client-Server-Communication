#pragma once


#include "pch.h"


enum class JobPriority
{
	HIGH,
	LOW,
	STREAMING,						// single thread
	COUNT
};


struct JobArgs
{
	int32_t							_jobIndex				= 0;
	int32_t							_groupId				= 0;
	int32_t							_groupIndex				= 0;

	void*							_sharedMemory			= nullptr;
	bool							_isFirstJobInGroup		= false;
	bool							_isLastJobInGroup		= false;
};


struct JobContext
{
	std::atomic< int32_t >			_counter{ 0 };
	JobPriority						_priority				= JobPriority::HIGH;
};