#pragma once


#include "../MovePrediction/PacketId.h"
#include "../MovePrediction/RecvBuffer.h"


class RIOSession;

using PacketHandler									= void( * )( RIOSession* sesion, const PacketView& packetView );


class PacketDispatcher
{
public:
	PacketDispatcher( void ) noexcept
	{
		
	}

	void											registerHandler( PacketId id, PacketHandler handler ) noexcept
	{
		_handlers.emplace( static_cast< int32_t >( id ), /*std::move*/( handler ) );
	}

	void											dispatch( RIOSession* session, const PacketView& packetView ) noexcept
	{
		auto it										= _handlers.find( static_cast< int32_t >( packetView._id ) );
		if (it != _handlers.end())
		{
			it->second( session, packetView );
		}
		else
		{
			HDASSERT( false, "등록 하지 않은 Packet Handler를 요청 했습니다." );
			// unknown packet
		}
	}


private:
	std::unordered_map< int32_t, PacketHandler >	_handlers;
};

void												registerPacketHandlers( PacketDispatcher& dispatcher ) noexcept;


// 나중에 별도로 .h 파일로 구분 필요

void												handlePlayerJoinReq( RIOSession* session, const PacketView& packet ) noexcept;
void												handlePlayerLeaveReq( RIOSession* session, const PacketView& packet ) noexcept;
void												handlePlayerInputReq( RIOSession* session, const PacketView& packet ) noexcept;
