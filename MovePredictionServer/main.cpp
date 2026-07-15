#include "pch.h"
#include "RIOSessionManager.h"
#include "RIOManager.h"
#include "GameServer.h"
#include "JobSystem.h"


int main( void )
{
	//SetUnhandledExceptionFilter( ExceptionFilter );

	_sessionManager							= new RIOSessionManager();
	_rioManager								= new RIOManager();

	GameServer* gameServer					= new GameServer();
	gameServer->getJobSystem().initialize( 4 );

	ServerPacketDispatcher& dispatcher		= gameServer->getPacketDispatcher();
	dispatcher.setContext( gameServer );
	registerPacketHandlers2( dispatcher );

	if ( false == _rioManager->initialize() )
	{
		return 0;
	}

	if ( false == _sessionManager->initializeAcceptSessionPool( gameServer ) )
	{
		return 0;
	}

	if ( false == _rioManager->startIOThreads() )
	{
		return 0;
	}

	if ( false == _rioManager->startAcceptLoop() )
	{
		return 0;
	}

	gameServer->getJobSystem().shutdown();

	return 0;
}