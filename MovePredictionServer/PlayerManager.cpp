#include "pch.h"
#include "PlayerManager.h"
#include "Player.h"


PlayerManager::PlayerManager( int32_t poolSize ) noexcept
{
	_playerPool.reserve( poolSize );
	_freeList.reserve( poolSize );

	for ( int32_t ii = 0; ii < poolSize; ++ii )
	{
		std::unique_ptr< Player > player				= std::make_unique< Player >();
		_freeList.emplace_back( player.get() );
		_playerPool.emplace_back( std::move( player ) );
	}
}

PlayerManager::~PlayerManager( void ) noexcept
{

}

Player* PlayerManager::addPlayer( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "세션 정보가 비정상 인데 Player를 등록하면 안됩니다." );

	{
		AutoReadLocker locker( &_lock );
		auto it											= _sessionToPlayer.find(session);
		if ( it != _sessionToPlayer.end() )
		{
			return it->second;
		}
	}

	// 없는 경우 Player Pool에서 가져와야 한다
	Player* player										= nullptr;
	{
		AutoWriteLocker locker( &_lock );

		player											= allocPlayer();
		player->_playerId								= _generator.generate();
		player->_session								= session;

		_activePlayers[ player->_playerId ]				= player;
		_sessionToPlayer[ session ]						= player;
	}

	return player;
}

void PlayerManager::removePlayer( uint32_t playerId ) noexcept
{
	HDASSERT( 0 < playerId, "PlayerId가 비정상 입니다. ");

	{
		AutoWriteLocker locker( &_lock );

		auto it											= _activePlayers.find( playerId );
		if ( it == _activePlayers.end() )
		{
			HDASSERT( false, "remove Player 대상이 비정상 입니다. ");
			return;
		}

		Player* player									= it->second;
		_sessionToPlayer.erase( player->_session );
		_activePlayers.erase( it );
    
		freePlayer( player );
	}
}

void PlayerManager::removePlayerBySession( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "Player Session이 비정상 입니다. ");

	{
		AutoWriteLocker locker( &_lock );

		auto it											= _sessionToPlayer.find( session );
		if ( it == _sessionToPlayer.end() )
		{
			HDASSERT( false, "remove Player 대상이 비정상 입니다. ");
			return;
		}

		Player* player									= it->second;
		_activePlayers.erase( player->_playerId );
		_sessionToPlayer.erase( it );

		freePlayer( player );
	}
}

Player* PlayerManager::getPlayer( int32_t playerId ) noexcept
{
	HDASSERT( 0 < playerId, "PlayerId가 비정상 입니다. ");

	{
		AutoReadLocker locker( &_lock );

		 auto it										= _activePlayers.find( playerId );
		if ( it == _activePlayers.end() )
		{
			return nullptr;
		}
			
		return it->second;
	}
}

Player* PlayerManager::getPlayerBySession( RIOSession* session ) noexcept
{
	HDASSERT( nullptr != session, "Player Session이 비정상 입니다. ");

	{
		AutoReadLocker locker( &_lock );

		 auto it										= _sessionToPlayer.find( session );
		if ( it == _sessionToPlayer.end() )
		{
			return nullptr;
		}
			
		return it->second;
	}
}

int32_t PlayerManager::getPlayerCount( void ) noexcept
{
	{
		AutoReadLocker locker( &_lock );

		return _activePlayers.size();
	}
}

Player* PlayerManager::allocPlayer( void ) noexcept
{
	// 락 걸고 사용해야 합니다.
	if ( true == _freeList.empty() )
	{
		return nullptr;
	}

	Player* player										= _freeList.back();

	_freeList.pop_back();
	return player;
}

void PlayerManager::freePlayer( Player* player ) noexcept
{
	*player												= {};
	_freeList.emplace_back( player );
}
