#include "pch.h"
#include "PacketHandler.h"

#include "../MovePrediction/RecvBuffer.h"
#include "../MovePrediction/Packet.h"

#include "RIOSession.h"


void registerPacketHandlers( PacketDispatcher& dispatcher ) noexcept
{
    dispatcher.registerHandler( PacketId::PLAYER_JOIN_REQ, handlePlayerJoinReq );
    dispatcher.registerHandler( PacketId::PLAYER_LEAVE_REQ, handlePlayerLeaveReq );
    dispatcher.registerHandler( PacketId::PLAYER_INPUT_REQ, handlePlayerInputReq );
}

void handlePlayerJoinReq( RIOSession* session, const PacketView& packet ) noexcept
{
    const PlayerJoinReq* req                                    = reinterpret_cast< const PlayerJoinReq* >( packet._data + PACKET_HEADER_SIZE );

    Logger::log( LogLevel::DEBUG, "PlayerJoinReq : \n" );

    // TODO: 게임 로직 - 플레이어 생성, ID 할당 등

    PlayerJoinRes res;
    res._playerId                                               = 1;

    session->sendPacket( PacketId::PLAYER_JOIN_RES, res );
}

void handlePlayerLeaveReq( RIOSession* session, const PacketView& packet ) noexcept
{
    const PlayerLeaveReq* req = reinterpret_cast<const PlayerLeaveReq*>( packet._data + PACKET_HEADER_SIZE );

    Logger::log( LogLevel::DEBUG, "PlayerLeaveReq: playerId=%d\n", req->_playerId );

    // TODO: 게임 로직 - 플레이어 제거

    // TODO: 응답 전송
}

void handlePlayerInputReq( RIOSession* session, const PacketView& packet ) noexcept
{
    const PlayerInputReq* req = reinterpret_cast<const PlayerInputReq*>( packet._data + PACKET_HEADER_SIZE );

   // Logger::log( LogLevel::DEBUG, "PlayerInputReq: x=%.2f, y=%.2f, z=%.2f\n", req->_x, req->_y, req->_z );

    // TODO: 게임 로직 - 입력 처리, 이동 계산

    // TODO: 브로드캐스트
    // PlayerStateRes res;
    // res._playerId = session->getPlayerId();
    // res._x = newX;
    // res._y = newY;
    // res._z = newZ;
    // broadcastPacket( PacketId::PLAYER_STATE_RES, res );
}
