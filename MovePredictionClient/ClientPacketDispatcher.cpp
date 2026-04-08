#include "pch.h"
#include "ClientPacketDispatcher.h"

#include "../MovePrediction/Packet.h"

#include "ClientContext.h"


void registerClientHandlers( ClientPacketDispatcher& dispatcher ) noexcept
{
    dispatcher.registerHandler( PacketId::PLAYER_JOIN_RES, handlePlayerJoinRes );
    dispatcher.registerHandler( PacketId::PLAYER_LEAVE_RES, handlePlayerLeaveRes );
    dispatcher.registerHandler( PacketId::PLAYER_STATE_RES, handlePlayerStateRes );
}

void handlePlayerJoinRes( void* context, const Packet& packet ) noexcept
{
    const PlayerJoinRes* res                            = reinterpret_cast< const PlayerJoinRes* >( packet._data.data() );

    ClientContext* clientContext                        = reinterpret_cast< ClientContext* >( context );
    HDASSERT( nullptr != clientContext, "Context 데이터 값이 비정상 입니다." );

    if ( 0 != res->_playerId )
    {
        clientContext->_playerId                        = res->_playerId;
        std::cout << "[JOIN] 성공! playerId=" << res->_playerId << std::endl;

        // TODO: 게임 상태 업데이트
    }
    else
    {
        std::cout << "[JOIN] 실패!" << std::endl;
    }
}

void handlePlayerLeaveRes( void* context, const Packet& packet ) noexcept
{
    const PlayerLeaveRes* res                           = reinterpret_cast< const PlayerLeaveRes* >( packet._data.data() );
    
    ClientContext* clientContext                        = reinterpret_cast< ClientContext* >( context );
    HDASSERT( nullptr != clientContext, "Context 데이터 값이 비정상 입니다." );

    std::cout << "[LEAVE] playerId=" << res->_playerId <<  std::endl;

    // TODO: 플레이어 제거
}

void handlePlayerStateRes( void* context, const Packet& packet ) noexcept
{
    const PlayerStateRes* res                           = reinterpret_cast< const PlayerStateRes* >( packet._data.data() );

    ClientContext* clientContext                        = reinterpret_cast< ClientContext* >( context );
    HDASSERT( nullptr != clientContext, "Context 데이터 값이 비정상 입니다." );


    if ( clientContext->_playerId == res->_playerId )
    {
        // 나의 업데이트
        clientContext->_serverTargetPosition._x         = res->_x;
        clientContext->_serverTargetPosition._y         = res->_y;
        clientContext->_serverTargetPosition._z         = res->_z;
        clientContext->_hasServerTarget                 = true;

        std::cout << "[STATE] 내 위치 업데이트 pos=(" << res->_x << ", " << res->_y << ")" << std::endl;
    }
    else
    {
        int32_t slot                                    = -1;
        auto iter                                       = clientContext->_playerIdToSlot.find( res->_playerId );
        if ( iter == clientContext->_playerIdToSlot.end() )
        {
            // 없으면 추가
            slot                                        = clientContext->_nextSlot;
            clientContext->_nextSlot                    += 1;
            clientContext->_playerIdToSlot[ res->_playerId ]    = slot;
            clientContext->_playerPresents[ slot ]      = true;
        }
        else
        {
            slot                                        = iter->second;
        }

        // 상대방 위치 업데이트

        // 점진적 변화
        clientContext->_playerTargetPositions[ slot ]._x   = res->_x;
        clientContext->_playerTargetPositions[ slot ]._y   = res->_y;
        clientContext->_playerTargetPositions[ slot ]._z   = res->_z;

        // 즉시 반영
        clientContext->_playerSnapshotStates[ slot ]._yaw           = res->_yaw;
        clientContext->_playerSnapshotStates[ slot ]._pitch         = res->_pitch;
    }

    std::cout << "[STATE] playerId=" << res->_playerId << ", pos=(" << res->_x << ", " << res->_y << ", " << res->_z << ")" << std::endl;

    // TODO: 플레이어 위치 업데이트
}