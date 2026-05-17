#include "pch.h"
#include "SyncContextPool.h"

#include "RIOSession.h"


SyncContextPool::~SyncContextPool( void ) noexcept
{
	shutdown();
}

bool SyncContextPool::initialize( int32_t capacity ) noexcept
{
	HDASSERT( 0 < capacity, "Capactiy값이 비정상 입니다." );

	if ( true == _initialized )
	{
		HDASSERT( false, "초기화를 여러 번 할 수는 없습니다." );
		return false;
	}

	_pool.reserve( capacity );
	_freelist.reserve( capacity );

	for ( int32_t ii = 0; ii < capacity; ++ii )
	{
		RIOContext* context								= new RIOContext();

		_pool.push_back( context );
		_freelist.push_back( context );
	}

	_initialized										= true;

	Logger::log( LogLevel::DEBUG, "%s: pool initialized, capacity=%d\n", __FUNCTION__, capacity );

	return true;
}

void SyncContextPool::shutdown( void ) noexcept
{
	HDASSERT( true == _initialized, "Initialize를 하지 않고 shutdown을 하려고 합니다." );

	AutoWriteLocker locker( &_lock );

	for ( RIOContext* context : _pool )
	{
		delete context;
	}

	_pool.clear();
	_freelist.clear();

	_initialized										= false;
}

RIOContext* SyncContextPool::acquire( RIOSession* session, IOType ioType ) noexcept
{
	HDASSERT( nullptr != session, "Session 정보가 비정상 입니다." );

	RIOContext* context									= nullptr;

	{
		AutoWriteLocker locker( &_lock );

		if ( true == _freelist.empty() )
		{
			HDASSERT( false, "Free List가 공간이 없습니다." );
			return nullptr;
		}

		context											= _freelist.back();
		_freelist.pop_back();
	}

	// context
	context->_buf										= {};
	context->_session									= session;
	context->_ioType									= ioType;

	if ( nullptr != session )
	{
		// 디버깅용
		session->addReference();
	}

	return context;
}

void SyncContextPool::release( RIOContext* context ) noexcept
{
	HDASSERT( nullptr != context, "Context 정보가 비정상 입니다." );

	if ( nullptr != context->_session )
	{
		// 디버깅용
		context->_session->releaseReference();
		context->_session								= nullptr;
	}

	AutoWriteLocker locker( &_lock );
	_freelist.push_back( context );
}
