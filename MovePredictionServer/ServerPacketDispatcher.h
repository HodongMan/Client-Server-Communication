#pragma once


#include "../MovePrediction/PacketId.h"
#include "../MovePrediction/RecvBuffer.h"


class RIOSession;

using ServerPacketHandler							= void( * )( RIOSession* sesion, void* context, const Packet& packet );


class ServerPacketDispatcher
{
public:
	ServerPacketDispatcher( void ) noexcept
	{
		
	}

	void											setContext( void* context ) noexcept
	{
		HDASSERT( nullptr != context, "Context는 GameServer 입니다." );

		_context									= context;
	}

	void											registerHandler( PacketId id, ServerPacketHandler handler ) noexcept
	{
		_handlers.emplace( static_cast< int32_t >( id ), /*std::move*/( handler ) );
	}

	void											dispatch( RIOSession* session, const Packet& packet ) noexcept
	{
		auto it										= _handlers.find( static_cast< int32_t >( packet._id ) );
		if ( it != _handlers.end() )
		{
			it->second( session, _context, packet );
		}
		else
		{
			HDASSERT( false, "등록 하지 않은 Packet Handler를 요청 했습니다." );
			// unknown packet
		}
	}


private:
	void* _context									= nullptr;
	std::unordered_map< int32_t, ServerPacketHandler >	_handlers;
};
