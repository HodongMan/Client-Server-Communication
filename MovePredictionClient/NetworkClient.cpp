#include "pch.h"
#include "NetworkClient.h"


NetworkClient::NetworkClient( void ) noexcept
{

}

NetworkClient::~NetworkClient( void ) noexcept
{
	disconnect();
}

bool NetworkClient::connect( const char* ip, uint16_t port ) noexcept
{
	HDASSERT( nullptr != ip, "ip 정보가 비정상 입니다." );
	HDASSERT( 1024 < port, "port 정보가 비정상 입니다." );

	_socket										= socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( INVALID_SOCKET == _socket )
	{
		HDASSERT( false, "이런 socket 생성 실패는 발생할 수가 없다.... 비정상 입니다." );
		return false;
	}

	SOCKADDR_IN addr							= {};
	addr.sin_family								= AF_INET;
	addr.sin_port								= htons( port );
	inet_pton( AF_INET, ip, &addr.sin_addr );

	// blocking connect
	if ( SOCKET_ERROR == ::connect( _socket, ( SOCKADDR* )&addr, sizeof( addr ) ) )
	{
		HDASSERT( false, "connect 정보가 비정상 인듯 합니다. connection 실패" );
		::closesocket( _socket );
		_socket									= INVALID_SOCKET;
		return false;
	}

	// connection 성공 처리
	// 여기서 network thread 실행 한다

	_isRunning									= true;
	_threadHandle								= ( HANDLE )_beginthreadex( nullptr, 0, networkThread, this, 0, nullptr );
	if ( INVALID_HANDLE_VALUE == _threadHandle )
    {
		HDASSERT( false, "thread 생성에 실패라니 이런 일이 발생할 수 있는 것인가" );
		_isRunning								= false;
        ::closesocket( _socket );
        _socket									= INVALID_SOCKET;
        return false;
    }

	return true;
}

void NetworkClient::disconnect( void ) noexcept
{
	HDASSERT( INVALID_SOCKET != _socket, "Socket 연길이 되지 않았는데 disconnect 처리를 하는건 일단 고려하지 않았습니다." );
	HDASSERT( INVALID_HANDLE_VALUE != _threadHandle, "thread 생성이 되지 않았는데 disconnect 처리를 하는건 일단 고려하지 않았습니다." );
	HDASSERT( true == _isRunning, "thread 생성이 되지 않았는데 disconnect 처리를 하는건 일단 고려하지 않았습니다." );

	_isRunning									= false;

	::WaitForSingleObject( _threadHandle, 3000 );
	::CloseHandle( _threadHandle );
	_threadHandle								= INVALID_HANDLE_VALUE;

	::closesocket( _socket );
	_socket										= INVALID_SOCKET;


	// remove queue data;
	{
		AutoWriteLocker locker( &_sendLock );
		while ( false == _sendQueue.empty() )
		{
			_sendQueue.pop();
		}
	}

	{
		AutoWriteLocker locker( &_recvLock );
		while ( false == _recvQueue.empty() )
		{
			_recvQueue.pop();
		}
	}
}

bool NetworkClient::isConnected( void ) const noexcept
{
	return _isRunning;
}

void NetworkClient::send( const char* data, int32_t size ) noexcept
{
	HDASSERT( nullptr != data, "Send Data 정보가 비정상 입니다." );
	HDASSERT( 0 < size, "Data Size 정보가 비정상 입니다." );
	HDASSERT( INVALID_SOCKET != _socket, "Socket 연길이 되지 않았는데 send 처리를 하는건 일단 고려하지 않았습니다." );
	HDASSERT( INVALID_HANDLE_VALUE != _threadHandle, "thread 생성이 되지 않았는데 send 처리를 하는건 일단 고려하지 않았습니다." );
	HDASSERT( true == _isRunning, "thread 생성이 되지 않았는데 send 처리를 하는건 일단 고려하지 않았습니다." );

	if ( !isConnected() )
	{
		return;
	}
        

    std::vector<char> packet( data, data + size );

	{
		AutoWriteLocker locker( &_sendLock );
		_sendQueue.push( std::move( packet ) );
	}
}

bool NetworkClient::tryGetPacket( Packet& outPacket ) noexcept
{
	HDASSERT( INVALID_SOCKET != _socket, "Socket 연길이 되지 않았는데 packet을 가져오는건 일단 고려하지 않았습니다." );
	HDASSERT( INVALID_HANDLE_VALUE != _threadHandle, "thread 생성이 되지 않았는데 packet을 가져오는건 일단 고려하지 않았습니다." );
	HDASSERT( true == _isRunning, "thread 생성이 되지 않았는데 packet을 가져오는건 일단 고려하지 않았습니다." );

	{
		AutoReadLocker locker( &_recvLock );

		if ( _recvQueue.empty() )
		{
			return false;
		}
	}

	{
		AutoWriteLocker locker( &_recvLock );
		outPacket								= std::move( _recvQueue.front() );
		_recvQueue.pop();
	}

	return true;
}

unsigned int __stdcall NetworkClient::networkThread( LPVOID param ) noexcept
{
	NetworkClient* networkClient				= static_cast< NetworkClient* >( param );
	HDASSERT( nullptr != networkClient, "Network Client 정보가 비정상 입니다." );

	// nonblock
	u_long mode = 1;
	ioctlsocket( networkClient->_socket, FIONBIO, &mode );

	// select 데이터
	fd_set readSet;
	fd_set writeSet;

	while ( true == networkClient->_isRunning )
    {
		// 이중 초기화를 피하기 위해서 부득이하게 = {}; 처리 제거
        FD_ZERO( &readSet );
        FD_ZERO( &writeSet );

        FD_SET( networkClient->_socket, &readSet );

        // 보낼거 있으면 send queue도 확인하고 간다
        {
            AutoReadLocker locker( &networkClient->_sendLock );
            if ( !networkClient->_sendQueue.empty() )
			{
				FD_SET( networkClient->_socket, &writeSet );
			}
        }

        timeval timeout							= { 0, 10000 };  // 10ms
		int32_t result							= ::select( 0, &readSet, &writeSet, nullptr, &timeout );

        if ( SOCKET_ERROR == result )
        {
			// 보내기 실패하면 그냥 ASSERT 띄우고 계속하게 하자
			HDASSERT( false, "select 처리가 실패 했습니다. 비정상 입니다." );
            continue;
        }

        if ( result == 0 )
		{
			continue;
		}
            
        // Recv
        if ( FD_ISSET( networkClient->_socket, &readSet ) )
        {
			networkClient->processRecv();
        }

        // Send
        if ( FD_ISSET( networkClient->_socket, &writeSet ) )
        {
			networkClient->processSend();
        }
    }

	return 0;
}

void NetworkClient::processSend( void ) noexcept
{
	HDASSERT( INVALID_SOCKET != _socket, "Socket 연길이 되지 않았는데 send 처리는 일단 고려하지 않았습니다." );
	HDASSERT( INVALID_HANDLE_VALUE != _threadHandle, "thread 생성이 되지 않았는데 send 처리는 일단 고려하지 않았습니다." );
	HDASSERT( true == _isRunning, "thread 생성이 되지 않았는데 send 처리는 일단 고려하지 않았습니다." );

	{
		AutoWriteLocker locker( &_sendLock );

		while ( !_sendQueue.empty() )
		{
			std::vector< char >& packet			= _sendQueue.front();

			int32_t sent						= ::send( _socket, packet.data(), static_cast<int>( packet.size() ), 0 );

			if ( sent == SOCKET_ERROR )
			{
				if ( WSAGetLastError() == WSAEWOULDBLOCK )
				{
					// 다음에 다시
					break;
				}

				// 진짜 에러
				_isRunning						= false;
				break;
			}

			_sendQueue.pop();
		}
	}
}

void NetworkClient::processRecv( void ) noexcept
{
	HDASSERT( INVALID_SOCKET != _socket, "Socket 연길이 되지 않았는데 recv 처리는 일단 고려하지 않았습니다." );
	HDASSERT( INVALID_HANDLE_VALUE != _threadHandle, "thread 생성이 되지 않았는데 recv 처리는 일단 고려하지 않았습니다." );
	HDASSERT( true == _isRunning, "thread 생성이 되지 않았는데 recv 처리는 일단 고려하지 않았습니다." );

	char buf[ 4096 ];
    int32_t recvLen								= ::recv( _socket, buf, sizeof( buf ), 0 );

    if ( 0 < recvLen )
    {
        _recvBuffer.onRecv( buf, recvLen );

        AutoWriteLocker locker( &_recvLock );

        Packet packet							= {};
        while ( _recvBuffer.tryGetPacket( packet ) )
        {
            _recvQueue.push( std::move( packet ) );
        }
    }
    else if ( recvLen == 0 )
    {
        // 서버가 연결 끊음
        _isRunning								= false;
    }
    else
    {
        if ( WSAGetLastError() != WSAEWOULDBLOCK )
        {
            // 진짜 에러
            _isRunning							= false;
        }
    }
}
