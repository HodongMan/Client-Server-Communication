#pragma once


#include "TypeJob.h"


struct Job
{
	std::function< void( JobArgs ) >	_task;
	JobContext*							_context				= nullptr;

	int32_t								_groupId				= 0;
	int32_t								_groupJobOffset			= 0;
	int32_t								_groupJobEnd			= 0;
	int32_t								_sharedMemorySize		= 0;

	inline int32_t						execute( void ) noexcept
	{
		constexpr int32_t				MAX_SHARED_MEMORY		= 4096;

		JobArgs args;
		args._groupId					= _groupId;
		if ( 0 < _sharedMemorySize && _sharedMemorySize < MAX_SHARED_MEMORY )
		{
			args._sharedMemory			= alloca( _sharedMemorySize );
		}
		else
		{
			args._sharedMemory			= nullptr;
		}

		for ( int32_t ii = _groupJobOffset; ii < _groupJobEnd; ++ii )
		{
			args._jobIndex				= ii;
			args._groupIndex			= ii - _groupJobOffset;
			args._isFirstJobInGroup		= ( ii == _groupJobOffset );
			args._isLastJobInGroup		= ( ii == _groupJobEnd - 1 );

			_task( args );
		}

		return _context->_counter.fetch_sub( 1 );
	}
};


/*
HANDLE threadHandle		= ::GetCurrentThread();
assert( INVALID_HANDLE_VALUE != threadHandle );

PWSTR value;
HRESULT hr = GetThreadDescription( threadHandle, &value );
if ( true == SUCCEEDED( hr ) )
{
	printf( "%ls\n", value );
	LocalFree( value );
}
*/