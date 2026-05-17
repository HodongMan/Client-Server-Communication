#pragma once


#include "PacketHandler.h"

#include "RIOContextPool.h"
#include "SyncContextPool.h"


class RIOSession;
class RIOWorker;
struct RIOContext;


using ServerContextPool								= RIOContextPool< SyncContextPool >;

class RIOManager
{
public:
	RIOManager( void ) noexcept;
	~RIOManager( void ) noexcept;

	bool											initialize( void ) noexcept;

	bool											startIOThreads( void ) noexcept;
	bool											startAcceptLoop( void ) noexcept;

	RIO_CQ											getCompletionQueue( void ) const noexcept;

	HANDLE											getIocpHandle( void ) const noexcept;
	SOCKET											getListenSocket( void ) const noexcept;
	
	int32_t											getConnectionIndex( void ) const noexcept;
	void											processConnectionIndex( void ) noexcept;

	PacketDispatcher&								getPacketDispatcher( void ) noexcept;
	ServerContextPool&								getContextPool( void ) noexcept;

private:
	void											releaseIOThreads( void ) noexcept;

public:
	static RIO_EXTENSION_FUNCTION_TABLE				_rioFunctionTable;
	static LPFN_ACCEPTEX							_acceptEx;
	static LPFN_DISCONNECTEX						_disconnectEx;

	static char										_acceptBuf[ 64 ];

private:
	SOCKET											_listenSocket					= INVALID_SOCKET;
	HANDLE											_iocpHandle						= INVALID_HANDLE_VALUE;
	std::vector< RIOWorker* >						_workers;

	int32_t											_connectionIndex				= 0;

	PacketDispatcher								_dispatcher						= {};
	ServerContextPool								_contextPool;
};

extern RIOManager*									_rioManager;

void												releaseRIOContext( RIOContext* context ) noexcept;

BOOL												disconnectEx( SOCKET socket, LPOVERLAPPED lpOverlapped, DWORD flags, DWORD reserved );

BOOL												acceptEx( SOCKET listenSocket, SOCKET acceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped );
