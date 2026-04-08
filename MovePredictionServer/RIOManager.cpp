#include "pch.h"
#include "RIOManager.h"

#include "RIOContext.h"
#include "RIOSessionManager.h"
#include "RIOSession.h"
#include "RIOWorker.h"


#define LISTEN_PORT	5000

RIO_EXTENSION_FUNCTION_TABLE RIOManager::_rioFunctionTable;

LPFN_DISCONNECTEX RIOManager::_disconnectEx				= nullptr;
LPFN_ACCEPTEX RIOManager::_acceptEx						= nullptr;

char RIOManager::_acceptBuf[ 64 ]						= { 0, };

RIOManager* _rioManager									= nullptr;



RIOManager::RIOManager( void ) noexcept
{
	WSAData wsaData										= {};
	if ( 0 != ::WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: WSAStartup error\n", __FUNCTION__ );
		return;
	}
}

RIOManager::~RIOManager( void ) noexcept
{
	releaseIOThreads();
	HDASSERT( INVALID_HANDLE_VALUE != _iocpHandle, "IOCP HANDLE이 정리 전에 INVALID 하다니 비정상 입니다." );
	::CloseHandle( _iocpHandle );

	::WSACleanup();
}

bool RIOManager::initialize( void ) noexcept
{
	// listen socket은 completion port 기능만 사용하더라도 WSA_FLAG_REGISTERED_IO 적용은 필수입니다.
	// 만약 적용하지 않으면 completion 결과를 받을 수 없음
	_listenSocket										= ::WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_REGISTERED_IO | WSA_FLAG_OVERLAPPED );
	HDASSERT( INVALID_SOCKET != _listenSocket, "WSASocket 생성이 비정상 입니다." );

	// socket opt 1 reuse 2 nodelay 3 ack_frequency
	BOOL reuse = TRUE;
	setsockopt( _listenSocket, SOL_SOCKET, SO_REUSEADDR, ( const char* )&reuse, sizeof( reuse ) );

	BOOL nodelay = TRUE;
	setsockopt( _listenSocket, IPPROTO_TCP, TCP_NODELAY, ( const char* )&nodelay, sizeof( nodelay ) );

	int32_t ackFrequency                = 1;
    DWORD bytes                         = 0;

	int32_t result                      = ::WSAIoctl( _listenSocket, SIO_TCP_SET_ACK_FREQUENCY, &ackFrequency, sizeof( ackFrequency ), nullptr, 0, &bytes, nullptr, nullptr );
	HDASSERT( SOCKET_ERROR != result, "WSAIoctl의 SIO_TCP_SET_ACK_FREQUENCY 처리가 비정상 입니다." );

	_iocpHandle							= ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	HDASSERT( INVALID_HANDLE_VALUE != _iocpHandle, "CreateIoCompletionPort 생성이 비정상 입니다." );

	//join
	HANDLE resultHandle					= ::CreateIoCompletionPort( reinterpret_cast< HANDLE >( _listenSocket ), _iocpHandle, 0, 0 );
	HDASSERT( resultHandle == _iocpHandle, "CreateIoCompletionPort 생성 중 Join 처리가 비정상 입니다."  );

	// bind
	SOCKADDR_IN serverAddress			= {};
	//ZeroMemory( &serverAddress, sizeof( serverAddress ) );
	serverAddress.sin_family			= AF_INET;
	serverAddress.sin_port				= htons( LISTEN_PORT );
	serverAddress.sin_addr.s_addr		= htonl( INADDR_ANY );

	if ( SOCKET_ERROR == ::bind( _listenSocket, ( SOCKADDR* )&serverAddress, sizeof( serverAddress ) ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: socket bind error\n", __FUNCTION__ );
		return false;
	}

	// acceptex
	GUID guidAccept						= WSAID_ACCEPTEX;
	if ( SOCKET_ERROR == ::WSAIoctl( _listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAccept, sizeof( GUID ), &_acceptEx, sizeof( LPFN_ACCEPTEX ), &bytes, nullptr, nullptr ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: WSAIoctl WSAID_ACCEPTEX error\n", __FUNCTION__ );
		return false;
	}

	// disconnectex
	GUID guidDisconnect					= WSAID_DISCONNECTEX;
	if ( SOCKET_ERROR == ::WSAIoctl( _listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnect, sizeof( GUID ), &_disconnectEx, sizeof( LPFN_DISCONNECTEX ), &bytes, nullptr, nullptr ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: WSAIoctl WSAID_DISCONNECTEX error\n", __FUNCTION__ );
		return false;
	}

	// riofunctiontable
	GUID guidRIOFunctionTable			= WSAID_MULTIPLE_RIO;
	bytes								= 0;
	if ( SOCKET_ERROR == ::WSAIoctl( _listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &guidRIOFunctionTable, sizeof( GUID ), & _rioFunctionTable, sizeof(_rioFunctionTable), &bytes, nullptr, nullptr))
	{
		Logger::log( LogLevel::ERRORS, "%s error: WSAIoctl WSAID_MULTIPLE_RIO error\n", __FUNCTION__ );
		return false;
	}

	registerPacketHandlers( _dispatcher );

	return true;
}

bool RIOManager::startIOThreads( void ) noexcept
{
	_workers.reserve( IO_THREAD_COUNT );

	for ( int32_t ii = 0; ii < IO_THREAD_COUNT; ++ii )
	{
		RIOWorker* worker				= new RIOWorker();
		HDASSERT( nullptr != worker, "RIO Worker 생성이 비정상 입니다." );

		if ( false == worker->start( ii ) )
		{
			releaseIOThreads();
			return false;
		}

		_workers.emplace_back( worker );
	}

	Logger::log( LogLevel::ERRORS, "%s RIO IO Thread Started : Thread Count %d started \n", __FUNCTION__, IO_THREAD_COUNT );

	return true;
}

bool RIOManager::startAcceptLoop( void ) noexcept
{
	// todo Accept Thread 만들기
	if ( SOCKET_ERROR == listen( _listenSocket, SOMAXCONN ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: listen error\n", __FUNCTION__ );
		return false;
	}

	while ( _sessionManager->acceptSessions() )
	{
		Sleep( 1 );
	}

	return true;
}

RIO_CQ RIOManager::getCompletionQueue( void ) const noexcept
{
	const int32_t selectedThreadIndex	= getConnectionIndex() % _workers.size();
	HDASSERT( 0 <= selectedThreadIndex, "Thread Index를 얻는데 음수가 나오다니 비정상 입니다." );

	return _workers[ selectedThreadIndex ]->getCompletionQueue();
}

HANDLE RIOManager::getIocpHandle( void ) const noexcept
{
	HDASSERT( INVALID_HANDLE_VALUE != _iocpHandle, "IOCP HANDLE은 초기화 이후에 사용해야 하는 함수 입니다." );

	return _iocpHandle;
}

SOCKET RIOManager::getListenSocket( void ) const noexcept
{
	HDASSERT( INVALID_SOCKET != _listenSocket, "LISTEN SOCKET은 초기화 이후에 사용해야 하는 함수 입니다." );

	return _listenSocket;
}

int32_t RIOManager::getConnectionIndex( void ) const noexcept
{
	return _connectionIndex;
}

void RIOManager::processConnectionIndex( void ) noexcept
{
	_connectionIndex									+= 1;
}

PacketDispatcher& RIOManager::getPacketDispatcher( void ) noexcept
{
	return _dispatcher;
}

void RIOManager::releaseIOThreads( void ) noexcept
{
	for ( RIOWorker* worker : _workers )
	{
		delete worker;
	}
}

BOOL disconnectEx( SOCKET socket, LPOVERLAPPED lpOverlapped, DWORD flags, DWORD reserved )
{
	return RIOManager::_disconnectEx( socket, lpOverlapped, flags, reserved );
}

BOOL acceptEx( SOCKET listenSocket, SOCKET acceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped )
{
	return RIOManager::_acceptEx( listenSocket, acceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped );
}
