#include "CommonPch.h"
#include "LinearAllocator.h"


LinearAllocator::LinearAllocator( int64_t size ) noexcept
{
	_memory									= new int8_t[ size ];
	_next									= _memory;
	_bytesRemaining							= size;
}

LinearAllocator::~LinearAllocator( void ) noexcept
{
	delete _memory;
}

int8_t* LinearAllocator::alloc( int64_t size ) noexcept
{
	assert( size <= _bytesRemaining );
	int8_t* memory							= _next;

	assert( nullptr != memory );

	_next									+= size;
	_bytesRemaining							-= size;

	return memory;
}
