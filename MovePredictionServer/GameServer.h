#pragma once

#include "PlayerManager.h"
#include "ServerPacketHandler.h"

class GameServer
{
public:
	GameServer( void ) noexcept;
	~GameServer( void ) noexcept;

	GameServer ( const GameServer& ) noexcept = delete;
	GameServer&							operator=( const GameServer& ) noexcept = delete;

	PlayerManager&						getPlayerManager( void ) noexcept;
	ServerPacketDispatcher&				getPacketDispatcher( void ) noexcept;

private:
	PlayerManager						_playerManager;
	ServerPacketDispatcher				_dispatcher;
};