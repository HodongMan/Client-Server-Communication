#pragma once


#include "pch.h"


enum class PacketType
{
	NONE,
	MAX,
};

struct IPEndPoint
{
	int32_t						_address		= 0;
	int16_t						_port			= 0;

	IPEndPoint( uint8_t a, uint8_t b, uint8_t c, uint8_t d, int16_t port ) noexcept
	{
	
	}

	bool						equal( const IPEndPoint& rhs ) noexcept
	{
	
	}

	SOCKADDR_IN					toSockAddrIn( void ) const noexcept
	{
	
	}

	std::string					toString( void ) noexcept
	{
		
	}
};