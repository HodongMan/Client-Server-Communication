#pragma once


#include "PacketHeader.h"

struct PacketView
{
	int32_t						_id						= 0;
	int32_t						_size					= 0;
	const char*					_data					= nullptr;
};


class RecvBuffer
{
public:
	RecvBuffer( int32_t capacity = RECV_BUFFER_SIZE ) noexcept;
	~RecvBuffer( void ) noexcept;

	RecvBuffer( const RecvBuffer& ) = delete;
	RecvBuffer&					operator=( const RecvBuffer& ) = delete;

	void						onRecv( const char* data, int32_t size ) noexcept;
	void						onRecv( int32_t size ) noexcept;
	
	bool						tryGetPacket( Packet& outPacket ) noexcept;
	bool						tryGetPacket( PacketView& outPacket ) noexcept;

	void						compact( void ) noexcept;

	char*						getWritePtr( void ) noexcept;
	int32_t						getFreeSize( void ) const noexcept;

private:
	char*						_buffer					= nullptr;
	int32_t						_capacity				= 0;
	
	// read Index;
	int32_t						_readPosition			= 0;

	// write Index;
	// 순서대로 읽기 때문에 선형적으로 늘고 compact 기능 사용
	int32_t						_writePosition			= 0;
};