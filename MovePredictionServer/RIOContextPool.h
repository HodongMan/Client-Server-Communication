#pragma once


#include "RIOContext.h"
#include "TypeRIO.h"


template < typename PoolImpl >
class RIOContextPool
{
public:
	RIOContextPool( void ) noexcept = default;
	~RIOContextPool( void ) noexcept = default;

	RIOContextPool( const RIOContextPool& ) noexcept = delete;
	RIOContextPool&								operator=( const RIOContextPool& ) noexcept = delete;

	bool										initialize( int32_t capacity ) noexcept
	{
		//HDASSERT( 0 < "Capacity 값이 비정상 입니다." );

		return _impl.initialize( capacity );
	}

	RIOContext*									acquire( RIOSession* session, IOType ioType ) noexcept
	{
		//HDASSERT( nullptr != session, "Session 정보가 비정상 입니다." );

		return _impl.acquire( session, ioType );
	}

	void										release( RIOContext* context ) noexcept
	{
		//HDASSERT( nullptr != context, "Context 정보가 비정상 입니다." );

		_impl.release( context );
	}
	
private:
	PoolImpl									_impl;
};