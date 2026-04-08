#include "pch.h"
#include "RIOSessionManager.h"

#include "RIOContext.h"
#include "RIOSession.h"
#include "GameServer.h"


RIOSessionManager* _sessionManager				= nullptr;


RIOSessionManager::RIOSessionManager( void ) noexcept
{

}

RIOSessionManager::~RIOSessionManager( void ) noexcept
{

}

bool RIOSessionManager::initializeAcceptSessionPool( GameServer* gameServer ) noexcept
{
	HDASSERT( nullptr != gameServer, "GameServer 데이터가 비정상 입니다." );

	_reserveSessions.reserve( MAX_CLIENTS );

	for ( int32_t ii = 0; ii < MAX_CLIENTS; ++ii )
	{
		// 세션을 한번에 MAX_CLIENTS 개수만큼 할당하고 넣는게 나을거같은데....
		// todo 로 나중에 수정하자
		RIOSession* session					= new RIOSession();
		if ( false == session->initialize( gameServer ) )
		{
			deleteReserveSession();
			return false;
		}

		_reserveSessions.emplace_back( session );
	}

	return true;
}

bool RIOSessionManager::acceptSessions( void ) noexcept
{
	HDASSERT( 0 <= _currentAcceptCount, "해당 값은 음수가 아닙니다 비정상 입니다" );
	HDASSERT( _currentAcceptCount <= MAX_CLIENTS, "MAX값보다 더 많이 연결될 수는 없습니다 비정상 입니다." );
	//HDASSERT( _reserveSessions.size() == MAX_CLIENTS, "MAX값 만큼만 미리 할당합니다. 더 적거나 크면은 처리가 안됩니다." );

	AutoWriteLocker locker( &_lock );

	while ( _currentAcceptCount < MAX_CLIENTS )
	{
		RIOSession* newSession				= _reserveSessions.back();
		_reserveSessions.pop_back();

		_currentAcceptCount					+= 1;

		newSession->addReference();

		if ( false == newSession->postAccept() )
		{
			return false;
		}
	}

	return true;
}

void RIOSessionManager::returnClientSession( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "Return Session 정보가 비정상 입니다." );

	AutoWriteLocker locker( &_lock );

	// 세션을 정리하고 반납하는지 확인 필요
	_reserveSessions.emplace_back( session );

	_currentAcceptCount						-= 1;
}

void RIOSessionManager::deleteReserveSession( void ) noexcept
{
	for ( RIOSession* reserveSession : _reserveSessions )
	{
		HDASSERT( nullptr != reserveSession, "Reserve Session 정보가 비정상 입니다." );
		delete reserveSession;
	}

	_reserveSessions.clear();
}

void RIOSessionManager::addActiveSession( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "Active Session 정보가 비정상 입니다." );

	{
		AutoWriteLocker locker( &_activeLock );
		_activeSessions.insert( session );
	}
}

void RIOSessionManager::removeActiveSession( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "Active Session 정보가 비정상 입니다." );

	{
		AutoWriteLocker locker( &_activeLock );
		_activeSessions.erase( session );
	}
}
