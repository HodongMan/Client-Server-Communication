#pragma once


#include "../MovePrediction/PacketId.h"
#include "../MovePrediction/PacketHeader.h"


using ClientPacketHandler								= void( * )( void* context, const Packet& packet ) noexcept;


class ClientPacketDispatcher
{
public:
    void                                                setContext( void* context ) noexcept
    {
        HDASSERT( nullptr != context, "Context값이 비정상 입니다." );
        _context                                        = context;
    }

    void                                                registerHandler( PacketId id, ClientPacketHandler handler ) noexcept
    {
        _handlers.emplace( static_cast<int32_t>( id ), handler );
    }

    bool dispatch( const Packet& packet ) noexcept
    {
        auto it                                         = _handlers.find( packet._id );
        if ( it == _handlers.end() )
        {
            return false;
        }

        it->second( _context, packet );

        return true;
    }
private:
    void*                                               _context                        = nullptr;
	std::unordered_map< int32_t, ClientPacketHandler >	_handlers;
};

void                                                    registerClientHandlers( ClientPacketDispatcher& dispatcher ) noexcept;

// 별도의 idl로 빼야함

void                                                    handlePlayerJoinRes( void* context, const Packet& packet ) noexcept;
void                                                    handlePlayerLeaveRes( void* context, const Packet& packet ) noexcept;
void                                                    handlePlayerStateRes( void* context, const Packet& packet ) noexcept;