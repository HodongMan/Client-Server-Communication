#pragma once


struct RIOContext;


constexpr int32_t						IO_THREAD_COUNT					= 4;

constexpr int32_t						RIO_COMPLETION_KEY				= 0xC0DE;

constexpr int32_t						SESSION_BUFFER_SIZE				= 65536;
constexpr int32_t						MAX_RIO_RESULT					= 256;
constexpr int32_t						MAX_RQ_SEND_SIZE				= 32;
constexpr int32_t						MAX_RQ_RECV_SIZE				= 4;
constexpr int32_t						MAX_CLIENTS						= 5000;
constexpr int32_t						MAX_CQ_SIZE						= ( MAX_RQ_SEND_SIZE + MAX_RQ_RECV_SIZE ) * MAX_CLIENTS / IO_THREAD_COUNT;



enum class IOType
{
	NONE,
	SEND,
	RECV,
	ACCEPT,
	DISCONNECT,
};

enum class DisconnectReason
{
	NONE,
	ACTIVE,
};


void												releaseRIOContext( RIOContext* context ) noexcept;
