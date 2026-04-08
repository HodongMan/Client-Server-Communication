#pragma once


#include "../MovePrediction/PacketHeader.h"
#include "../MovePrediction/RecvBuffer.h"
#include "RWLock.h"


class NetworkClient
{
public:
	NetworkClient( void ) noexcept;
	~NetworkClient( void ) noexcept;


	bool									connect( const char* ip, uint16_t port ) noexcept;
	void									disconnect( void ) noexcept;

	bool									isConnected( void ) const noexcept;

	void									send( const char* data, int32_t size ) noexcept;
	bool									tryGetPacket( Packet& outPacket ) noexcept;

private:
	static unsigned int WINAPI				networkThread( LPVOID param ) noexcept;

	void									processSend( void ) noexcept;
	void									processRecv( void ) noexcept;

private:
	SOCKET									_socket				= INVALID_SOCKET;
	HANDLE									_threadHandle		= INVALID_HANDLE_VALUE;

	volatile bool							_isRunning			= false;

	std::queue< std::vector< char > >		_sendQueue;
	RWLock									_sendLock;

	std::queue< Packet >					_recvQueue;
	RWLock									_recvLock;
	
	RecvBuffer								_recvBuffer;
};