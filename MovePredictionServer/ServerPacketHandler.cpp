#include "pch.h"
#include "ServerPacketHandler.h"


#include "../MovePrediction/RecvBuffer.h"
#include "../MovePrediction/Packet.h"
#include "../MovePrediction/PacketUtil.h"

#include "RIOSession.h"
#include "ServerPacketDispatcher.h"
#include "GameServer.h"
#include "Player.h"


void registerPacketHandlers2( ServerPacketDispatcher& dispatcher ) noexcept
{
    dispatcher.registerHandler( PacketId::PLAYER_JOIN_REQ, handlePlayerJoinReq2 );
    dispatcher.registerHandler( PacketId::PLAYER_LEAVE_REQ, handlePlayerLeaveReq2 );
    dispatcher.registerHandler( PacketId::PLAYER_INPUT_REQ, handlePlayerInputReq2 );
}

void handlePlayerJoinReq2( RIOSession* session, void* context, const Packet& packet ) noexcept
{
    GameServer* server                                          = static_cast< GameServer* >( context );
    HDASSERT( nullptr != server, "GameServer 데이터가 비정상 입니다." );

    const PlayerJoinReq* req                                    = reinterpret_cast< const PlayerJoinReq* >( packet._data.data() );

    Logger::log( LogLevel::DEBUG, "PlayerJoinReq : \n" );

    Player* player                                              = server->getPlayerManager().addPlayer( session );

    PlayerJoinRes res;
    res._playerId                                               = player->_playerId;

    session->sendPacket( PacketId::PLAYER_JOIN_RES, res );
}

void handlePlayerLeaveReq2( RIOSession* session, void* context, const Packet& packet ) noexcept
{
    GameServer* server                                          = static_cast< GameServer* >( context );
    HDASSERT( nullptr != server, "GameServer 데이터가 비정상 입니다." );

    const PlayerLeaveReq* req                                    = reinterpret_cast< const PlayerLeaveReq* >( packet._data.data() );

    Player* player                                              = server->getPlayerManager().getPlayerBySession( session );
    HDASSERT( nullptr != player, "Session에 연결된 Player가 비정상 입니다." );

    server->getPlayerManager().removePlayer( player->_playerId );

    Logger::log( LogLevel::DEBUG, "[LEAVE] playerId=%d\n", player->_playerId );

    PlayerLeaveRes res;
    res._playerId                                               = player->_playerId;

    session->sendPacket( PacketId::PLAYER_LEAVE_RES, res );

    // TODO: 응답 전송
}

void handlePlayerInputReq2( RIOSession* session, void* context, const Packet& packet ) noexcept
{
    const PlayerInputReq* req                                   = reinterpret_cast<const PlayerInputReq*>( packet._data.data() );

    GameServer* server                                          = static_cast< GameServer* >( context );
    HDASSERT( nullptr != server, "GameServer 데이터가 비정상 입니다." );

    Player* player                                              = server->getPlayerManager().getPlayerBySession( session );
    HDASSERT( nullptr != player, "Session에 연결된 Player가 비정상 입니다." );

    // 입력 복사
    player->_moveFlags                                          = req->_moveFlag;
    player->_yaw                                                = req->_yaw;
    player->_pitch                                              = req->_pitch;

    float cosYaw                                                = cosf( req->_yaw );
    float sinYaw                                                = sinf( req->_yaw );

    float dirX                                                  = 0.0f;
    float dirY                                                  = 0.0f;

    if ( req->_moveFlag & static_cast< int32_t >( MoveFlag::UP ) )
    {
        dirX                                                    += -sinYaw;
        dirY                                                    += cosYaw;
    }
    if ( req->_moveFlag & static_cast< int32_t >( MoveFlag::DOWN ) )
    {
        dirX                                                    -= -sinYaw;
        dirY                                                    -= cosYaw;
    }
    if ( req->_moveFlag & static_cast< int32_t >( MoveFlag::LEFT ) )
    {
        dirX                                                    -= cosYaw;
        dirY                                                    -= sinYaw;
    }
    if ( req->_moveFlag & static_cast< int32_t >( MoveFlag::RIGHT ) )
    {
        dirX                                                    += cosYaw;
        dirY                                                    += sinYaw;
    }

    // normalize and move
    float lengthSqaure                                          = dirX * dirX + dirY * dirY;
    if ( 0.0f < lengthSqaure )
    {
        float length                                            = sqrtf( lengthSqaure );
        dirX                                                    /= length;
        dirY                                                    /= length;

        constexpr float dt                                      = 0.1f;  // 100ms
        constexpr float PLAYER_MOVE_SPEED                       = 10.0f;

        player->_x                                              += dirX * PLAYER_MOVE_SPEED * dt;
        player->_y                                              += dirY * PLAYER_MOVE_SPEED * dt;
    }

    //Logger::log( LogLevel::DEBUG, "[INPUT] playerId=%d flags=%d pos=(%.2f, %.2f)\n", player->_playerId, req->_moveFlag, player->_x, player->_y );

    // 모든 플레이어에게 브로드캐스트
    PlayerStateRes response                                     = {};
    response._playerId                                          = player->_playerId;
    response._x                                                 = player->_x;
    response._y                                                 = player->_y;
    response._z                                                 = player->_z;
    //response._x                                                 = 0;
    //response._y                                                 = 0;
    //response._z                                                 = 0;
    response._pitch                                             = player->_pitch;
    response._yaw                                               = player->_yaw;
    /*
    static auto lastTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float actualDt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;

    Logger::log(LogLevel::DEBUG, "[INPUT] actualDt=%.4f hardcodedDt=0.1\n", actualDt);
    */
    server->getPlayerManager().forEach( [ &response ]( Player& player ) { player._session->sendPacket( PacketId::PLAYER_STATE_RES, response ); } );
}