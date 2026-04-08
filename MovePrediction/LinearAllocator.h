#pragma once


struct LinearAllocator
{
	int8_t*					_memory					= nullptr;
	int8_t*					_next					= nullptr;
	int64_t					_bytesRemaining			= 0;

	LinearAllocator( int64_t size ) noexcept;
	~LinearAllocator( void ) noexcept;
	int8_t*					alloc( int64_t size ) noexcept;
};