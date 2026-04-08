#pragma once


#include "pch.h"


constexpr uint16_t				PORT					= 5000;
constexpr int32_t				PACKET_BUDGET_PER_TICK	= 1024;
constexpr int32_t				MAX_CLIENTS				= 32;
constexpr int32_t				MAX_CLIENT_TICK_RATE	= 120;
constexpr int32_t				MAX_SERVER_TOCK_RATE	= 60;

constexpr int64_t				kilobytes( int32_t kb ) noexcept
{
	return kb * 1024;
}

constexpr int64_t				megabytes( int32_t mb ) noexcept
{
	return kilobytes( mb * 1024 );
}

constexpr int64_t				gigabytes( int32_t gb ) noexcept
{
	return megabytes( gb * 1024 );
}
