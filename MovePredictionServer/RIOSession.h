#pragma once

#include "RIOContext.h"
#include "Buffer.h"

#include "../MovePrediction/PacketId.h"
#include "../MovePrediction/RecvBuffer.h"


class GameServer;


class RIOSession
{
public:
	RIOSession( void ) noexcept;
	~RIOSession( void ) noexcept;

	bool										isConnected( void ) const noexcept;

	bool										postAccept( void ) noexcept;
	void										acceptCompletion( void ) noexcept;

	void										requestDisconnect( DisconnectReason reason ) noexcept;
	void										disconnectCompletion( void ) noexcept;

	bool										postRecv( void ) noexcept;
	void										recvCompletion( DWORD transferred ) noexcept;

	bool										postSend( const char* data, DWORD transferred ) noexcept;
	void										sendCompletion( DWORD transferred ) noexcept;

	void										addReference( void ) noexcept;
	void										releaseReference( void ) noexcept;

public:
	bool										initialize( GameServer* gameServer ) noexcept;

public:
	//send packet ฐüทร
	template< typename T >
	void										sendPacket( PacketId id, const T& packet ) noexcept
	{
		char buf[ 512 ];

		PacketHeader* header					= reinterpret_cast< PacketHeader* >( buf );
		header->_packetId						= static_cast< int16_t >( id );
		header->_size							= sizeof( PacketHeader ) + sizeof( T );

		::memcpy( buf + sizeof( PacketHeader ), &packet, sizeof( T ) );

		postSend( buf, header->_size );
	}

private:
	void										processPacket( const PacketView& packetView ) noexcept;

private:

	SOCKET										_socket					= INVALID_SOCKET;
	SOCKADDR_IN									_clientAddr				= {};

	volatile long								_referenceCount			= 0;

	RIO_BUFFERID								_rioBufferId			= RIO_INVALID_BUFFERID;
	Buffer										_rioCustomBuffer		= {};
	char*										_rioBuffer				= nullptr;
	RIO_RQ										_requestQueue			= RIO_INVALID_RQ;

	RecvBuffer									_recvBuffer				= {};

	volatile long								_isConnected			= 0;

	GameServer*									_gameServer				= nullptr;
};