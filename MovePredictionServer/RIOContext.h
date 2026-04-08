#pragma once


#include "TypeRIO.h"


class RIOSession;


struct RIOContext
{
	RIOContext( RIOSession* session, IOType ioType ) noexcept;

	RIO_BUF								_buf							= {};
	RIOSession*							_session						= nullptr;
	IOType								_ioType							= IOType::NONE;
};

struct OverlappedContext
{
	OverlappedContext( RIOSession* session, IOType ioType ) noexcept;

	OVERLAPPED							_overlapped						= {};
	RIOSession*							_session						= nullptr;
	IOType								_ioType							= IOType::NONE;
	WSABUF								_wsaBuf							= {};
};

struct OverlappedAcceptContext : public OverlappedContext
{
	OverlappedAcceptContext( RIOSession* session ) noexcept;
};

struct OverlappedDisconnectContext : public OverlappedContext
{
	OverlappedDisconnectContext( RIOSession* session, DisconnectReason disconnectReason ) noexcept;

	DisconnectReason					_reason							= DisconnectReason::NONE;
};