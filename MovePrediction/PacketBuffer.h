#pragma once


#include "CommonPch.h"
#include "TypePacket.h"


struct PacketBuffer
{
	int32_t						_index				= 0;
	int32_t						_size				= 0;
	uint8_t*					_packets			= nullptr;
	int32_t*					_packetSizes		= nullptr;
	IPEndPoint*					_endpoints			= nullptr;
	LARGE_INTEGER*				_times				= nullptr;
};

