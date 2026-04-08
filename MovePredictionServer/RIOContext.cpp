#include "pch.h"
#include "RIOContext.h"
#include "RIOSession.h"


RIOContext::RIOContext( RIOSession* session, IOType ioType ) noexcept
	: _session{ session }
	, _ioType{ ioType }
{
	session->addReference();
}

OverlappedContext::OverlappedContext( RIOSession* session, IOType ioType ) noexcept
	: _session{ session }
	, _ioType{ ioType }
{
	session->addReference();

	ZeroMemory( &_overlapped, sizeof( OVERLAPPED ) );
	ZeroMemory( &_wsaBuf, sizeof( WSABUF ) );
}

OverlappedAcceptContext::OverlappedAcceptContext( RIOSession* session ) noexcept
	: OverlappedContext( session, IOType::ACCEPT )
{

}

OverlappedDisconnectContext::OverlappedDisconnectContext( RIOSession* session, DisconnectReason disconnectReason ) noexcept
	: OverlappedContext( session, IOType::DISCONNECT )
	, _reason{ disconnectReason }
{

}
