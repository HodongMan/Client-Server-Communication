#include "pch.h"
#include "RIOSession.h"

#include "RIOManager.h"
#include "RIOSessionManager.h"
#include "GameServer.h"

#include "../MovePrediction/PacketHeader.h"


RIOSession::RIOSession( void ) noexcept
{

}

RIOSession::~RIOSession( void ) noexcept
{
	_rioManager->_rioFunctionTable.RIODeregisterBuffer( _rioBufferId );
	//::VirtualFreeEx( GetCurrentProcess(), _rioBuffer, 0, MEM_RELEASE );
}

bool RIOSession::initialize( GameServer* gameServer ) noexcept
{
	HDASSERT( nullptr != gameServer, "Game Server가 비정상 입니다." );

	SYSTEM_INFO systemInfo							= {};
	::GetSystemInfo( &systemInfo );

	const int64_t granularity						= systemInfo.dwAllocationGranularity;
	HDASSERT( 0 == SESSION_BUFFER_SIZE % granularity, "dwAllocationGranularity의 배수 증가 분 이여야 합니다." );

	//_rioBuffer										= reinterpret_cast< char* >( VirtualAllocEx( GetCurrentProcess(), 0, SESSION_BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE ) );
	//assert( nullptr != _rioBuffer );

	//_rioBufferId									= _rioManager->_rioFunctionTable.RIORegisterBuffer( _rioBuffer, SESSION_BUFFER_SIZE );
	_rioBufferId									= _rioManager->_rioFunctionTable.RIORegisterBuffer( _rioCustomBuffer.getBufferPtr(), SESSION_BUFFER_SIZE );
	HDASSERT( RIO_INVALID_BUFFERID != _rioBufferId, "RIORegisterBuffer 처리가 비정상 입니다." );

	_gameServer										= gameServer;

	return true;
}

void RIOSession::processPacket( const PacketView& packetView ) noexcept
{
	Logger::log( LogLevel::DEBUG, "%s packetId: %d, size: %d \n", __FUNCTION__, packetView._id, packetView._size );

	Packet packet									= {};
	packet._id										= packetView._id;
	packet._data.assign( packetView._data + PACKET_HEADER_SIZE, packetView._data + packetView._size );

	_gameServer->getPacketDispatcher().dispatch( this, packet );
}

bool RIOSession::isConnected( void ) const noexcept
{
	return !!_isConnected;
}

bool RIOSession::postAccept( void ) noexcept
{ 
	OverlappedAcceptContext* acceptContext			= new OverlappedAcceptContext( this );
	HDASSERT( nullptr != acceptContext, "OverlappedAcceptContext 생성이 비정상 입니다." );

	DWORD bytes										= 0;
	acceptContext->_wsaBuf.len						= 0;
	acceptContext->_wsaBuf.buf						= nullptr;

	// accept socket
	_socket											= ::WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_REGISTERED_IO );
	HDASSERT( INVALID_SOCKET != _socket, "WSASocket 생성이 비정상 입니다." );

	if ( FALSE == AcceptEx( _rioManager->getListenSocket(), _socket, _rioManager->_acceptBuf, 0, sizeof( SOCKADDR_IN ) + 16, sizeof( SOCKADDR_IN ) + 16, &bytes, ( LPOVERLAPPED )acceptContext ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			Logger::log( LogLevel::ERRORS, "%s error: AcceptEx error \n", __FUNCTION__ );
			delete acceptContext;
			
			return false;
		}
	}

	return true;
}

void RIOSession::acceptCompletion( void ) noexcept
{
	if ( true == ::InterlockedExchange( &_isConnected, 1 ) )
	{
		assert( false );
		return;
	}

	bool result										= true;
	do
	{
		SOCKET listenSocket							= _rioManager->getListenSocket();
		if ( SOCKET_ERROR == ::setsockopt( _socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ( char* )&listenSocket, sizeof( SOCKET ) ) )
		{
			Logger::log( LogLevel::ERRORS, "%s error: SO_UPDATE_ACCEPT_CONTEXT error \n", __FUNCTION__ );
			result									= false;

			break;
		}

		// socket opt 1 reuse 2 nodelay 3 ack_frequency
		BOOL reuse = TRUE;
		setsockopt( _socket, SOL_SOCKET, SO_REUSEADDR, ( const char* )&reuse, sizeof( reuse ) );

		BOOL nodelay = TRUE;
		setsockopt( _socket, IPPROTO_TCP, TCP_NODELAY, ( const char* )&nodelay, sizeof( nodelay ) );

		int32_t ackFrequency                = 1;
		DWORD bytes                         = 0;

		int32_t result                      = ::WSAIoctl( _socket, SIO_TCP_SET_ACK_FREQUENCY, &ackFrequency, sizeof( ackFrequency ), nullptr, 0, &bytes, nullptr, nullptr );
		assert( SOCKET_ERROR != result );

		int32_t addrLength					= sizeof( SOCKADDR_IN );
		if ( SOCKET_ERROR == ::getpeername( _socket, ( SOCKADDR* )&_clientAddr, &addrLength ) )
		{
			Logger::log( LogLevel::ERRORS, "%s error: getpeername error \n", __FUNCTION__ );
			result									= false;

			break;
		}

		RIO_CQ completionQueue						= _rioManager->getCompletionQueue();
		// request queue
		_requestQueue								= _rioManager->_rioFunctionTable.RIOCreateRequestQueue( _socket, MAX_RQ_RECV_SIZE, 1, MAX_RQ_SEND_SIZE, 1, completionQueue, completionQueue, nullptr );
		if ( RIO_INVALID_RQ == _requestQueue )
		{
			Logger::log( LogLevel::ERRORS, "%s error: RIOCreateRequestQueue error \n", __FUNCTION__ );
			result									= false;

			break;
		}

		// join
		HANDLE iocpHandle							= ::CreateIoCompletionPort( ( HANDLE)_socket, _rioManager->getIocpHandle(), ( ULONG_PTR )this, 0 );
		if ( iocpHandle != _rioManager->getIocpHandle() )
		{
			Logger::log( LogLevel::ERRORS, "%s error: CreateIoCompletionPort join error \n", __FUNCTION__ );
			result									= false;

			break;
		}

		_rioManager->processConnectionIndex();

		// active session 등록
		_sessionManager->addActiveSession( this );
	}
	while ( false );

	if ( false == result )
	{
		requestDisconnect( DisconnectReason::NONE );
		return;
	}

	Logger::log( LogLevel::DEBUG, "%s  Client Connected: \n", __FUNCTION__ );

	if ( false == postRecv() )
	{
		Logger::log( LogLevel::ERRORS, "%s error: postRecv error \n", __FUNCTION__ );
	}
}

void RIOSession::requestDisconnect( DisconnectReason reason ) noexcept
{
	if ( 0 == ::InterlockedExchange( &_isConnected, 0 ) )
	{
		return;
	}

	OverlappedDisconnectContext* disconnectContext	= new OverlappedDisconnectContext( this, reason );
	assert( nullptr != disconnectContext );

	if ( FALSE == disconnectEx( _socket, ( LPWSAOVERLAPPED )disconnectContext, TF_REUSE_SOCKET, 0 ) )
	{
		if ( WSA_IO_PENDING != ::WSAGetLastError() )
		{
			delete disconnectContext;
			Logger::log( LogLevel::ERRORS, "%s error: disconnectEx error \n", __FUNCTION__ );
		}
	}
}

void RIOSession::disconnectCompletion( void ) noexcept
{
	// 일단 session manager에서부터 삭제
	// 따라서 뒤에서 실패해서 롤백 이런거 안됩니다.
	// closesocket 뒤에 요청 날라오거나 그럴까봐 session에서 제외 먼저 합니다

	_sessionManager->removeActiveSession( this );

	LINGER lingerOption							= {};
	lingerOption.l_onoff						= 1;
	lingerOption.l_linger						= 0;

	if ( SOCKET_ERROR == ::setsockopt( _socket, SOL_SOCKET, SO_LINGER, ( const char* )&lingerOption, sizeof( LINGER ) ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: closesocket error \n", __FUNCTION__ );
	}

	Logger::log( LogLevel::DEBUG, "%s  Client Disconnected: \n", __FUNCTION__ );

	// if closesockeet request queue deleted
	::closesocket( _socket );

	_isConnected								= 0;
	_socket										= INVALID_SOCKET;
	
	releaseReference();
}

bool RIOSession::postRecv( void ) noexcept
{
	if ( false == isConnected() )
	{
		return false;
	}

	RIOContext* recvContext						= new RIOContext( this, IOType::RECV );
	assert( nullptr != recvContext );
	
	recvContext->_buf.BufferId					= _rioBufferId;
	recvContext->_buf.Length					= SESSION_BUFFER_SIZE;
	recvContext->_buf.Offset					= 0;

	DWORD recvBytes								= 0;
	DWORD flags									= 0;

	if ( FALSE == _rioManager->_rioFunctionTable.RIOReceive( _requestQueue, ( PRIO_BUF )recvContext, 1, flags, recvContext ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: RIOReceive error \n", __FUNCTION__ );
		releaseRIOContext( recvContext );

		return false;
	}

	return true;
}

void RIOSession::recvCompletion( DWORD transferred ) noexcept
{
	::memcpy( _recvBuffer.getWritePtr(), _rioCustomBuffer.getBufferPtr(), transferred );
	_recvBuffer.onRecv( transferred );

	PacketView packet;
	while ( true == _recvBuffer.tryGetPacket( packet ) )
	{
		processPacket( packet );
	}

	_recvBuffer.compact();
	postRecv();
}

bool RIOSession::postSend( const char* data, DWORD transferred ) noexcept
{
	if ( false == isConnected() )
	{
		return false;
	}

	RIOContext* sendContext						= new RIOContext( this, IOType::SEND );
	assert( nullptr != sendContext );

	constexpr int32_t SEND_OFFSET				= SESSION_BUFFER_SIZE / 2;
	::memcpy( _rioCustomBuffer.getBufferPtr() + SEND_OFFSET, data, transferred );

	sendContext->_buf.BufferId					= _rioBufferId;
	sendContext->_buf.Length					= transferred;
	sendContext->_buf.Offset					= SEND_OFFSET;

	DWORD sendBytes								= 0;
	DWORD flags									= 0;

	if ( FALSE == _rioManager->_rioFunctionTable.RIOSend( _requestQueue, ( PRIO_BUF )sendContext, 1, flags, sendContext ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: RIOSend error \n", __FUNCTION__ );
		releaseRIOContext( sendContext );

		return false;
	}

	return true;
}

void RIOSession::sendCompletion( DWORD transferred ) noexcept
{
	transferred;
}

void RIOSession::addReference( void ) noexcept
{
	::InterlockedIncrement( &_referenceCount );
}

void RIOSession::releaseReference( void ) noexcept
{
	long result								= ::InterlockedDecrement( &_referenceCount );
	if ( 0 == result )
	{
		_sessionManager->returnClientSession( this );
	}
}

