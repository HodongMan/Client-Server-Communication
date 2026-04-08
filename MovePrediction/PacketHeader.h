#pragma once


#include "CommonPch.h"


/*
#pragma pack( push, 1 )

struct PacketHeader
{
	int32_t							_checkCode				= 0;
	int32_t							_commandId				= 0;
	int32_t							_size					= 0;
	int32_t							_packetNo				= 0;
	int32_t							_sceneId				= 0;
	int64_t							_characterId			= 0;
};

#pragma pack( pop )

*/

#pragma pack( push, 1 )
struct PacketHeader
{
	int16_t							_size					= 0;
	int16_t							_packetId				= 0;
};

#pragma pack( pop )

struct Packet
{
	int16_t							_id						= 0;
	std::vector< char >				_data;
};

constexpr int32_t					RECV_BUFFER_SIZE		= 65536;
constexpr int32_t					PACKET_HEADER_SIZE		= sizeof( PacketHeader );