#pragma once


#include "RWLock.h"
#include "RIOSession.h"

#include "../MovePrediction/PacketId.h"


class GameServer;


class RIOSessionManager
{
public:
	RIOSessionManager( void ) noexcept;
	~RIOSessionManager( void ) noexcept;

	bool							initializeAcceptSessionPool( GameServer* gameServer ) noexcept;
	bool							acceptSessions( void ) noexcept;
	
	void							returnClientSession( RIOSession* session ) noexcept;
	void							deleteReserveSession( void ) noexcept;

	// active session 관련 처리
	void							addActiveSession( RIOSession* session ) noexcept;
	void							removeActiveSession( RIOSession* session ) noexcept;

	// 현재는 세션 전체 보내기만 지원 합니다.
	template< typename T >
	void							broadcastPacket( PacketId packetId, const T& packet ) noexcept;

	template< typename T >
	void							broadcastPacketExcept( PacketId packetId, const T& packet, RIOSession* except ) noexcept;

private:
	std::vector< RIOSession* >		_reserveSessions;
	int32_t							_currentAcceptCount			= 0;

	// session 전반에 쓰이는 RWLock
	RWLock							_lock;

	std::unordered_set< RIOSession* >	_activeSessions;
	// active session 관리에 쓰이는 RWLock
	RWLock							_activeLock;
};

extern RIOSessionManager*			_sessionManager;

template<typename T>
inline void RIOSessionManager::broadcastPacket( PacketId packetId, const T& packet ) noexcept
{
	std::vector< RIOSession* > sessions;
    {
        AutoReadLocker locker( &_activeLock );
        sessions.assign( _activeSessions.begin(), _activeSessions.end() );
    }

    for ( RIOSession* session : sessions )
    {
        session->sendPacket( packetId, packet );
    }
}

template<typename T>
inline void RIOSessionManager::broadcastPacketExcept( PacketId packetId, const T& packet, RIOSession* except ) noexcept
{
	std::vector< RIOSession* > sessions;
    {
        AutoReadLocker locker( &_activeLock );
        sessions.assign( _activeSessions.begin(), _activeSessions.end() );
    }

    for ( RIOSession* session : sessions )
    {
       if ( session != except )
		{
			session->sendPacket( packetId, packet );
		}
    }
}
