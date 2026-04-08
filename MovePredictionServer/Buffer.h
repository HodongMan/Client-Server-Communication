#pragma once


#include "TypeRIO.h"


class Buffer final
{
public:
	Buffer( int32_t size = SESSION_BUFFER_SIZE ) noexcept
	{
		HDASSERT( 0 < size, "size값은 0보다 커야 합니다 비정상 입니다." );
		HDASSERT( 0 == size % 2, "size값은 홀수는 안됩니다. 비정상 입니다." );
		// 2의 제곱을 체크하는 방법이 뭐가 있을까....

		_buffer					= reinterpret_cast< char* >( ::VirtualAlloc( nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
		HDASSERT( nullptr != _buffer, "메모리 할당 실패!" );
		_bufferSize				= size;
	}

	~Buffer( void ) noexcept
	{
		::VirtualFree( _buffer, 0, MEM_RELEASE );
	}

	char*						getBufferPtr( void ) const noexcept
	{
		return _buffer;
	}

	int32_t						getBufferSize( void ) const noexcept
	{
		return _bufferSize;
	}

private:
	char*						_buffer				= nullptr;
	int32_t						_bufferSize			= 0;
};