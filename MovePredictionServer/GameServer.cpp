#include "pch.h"
#include "GameServer.h"


GameServer::GameServer( void ) noexcept
{

}

GameServer::~GameServer( void ) noexcept
{

}

PlayerManager& GameServer::getPlayerManager( void ) noexcept
{
	return _playerManager;
}

ServerPacketDispatcher& GameServer::getPacketDispatcher( void ) noexcept
{
	return _dispatcher;
}
