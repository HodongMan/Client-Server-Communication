#pragma once


#include "../MovePrediction/Packet.h"


class RIOSession;


class Player
{
public:

public:
    int32_t                     _playerId       = 0;

    // 위치/방향
    float                       _x              = 0.0f;
    float                       _y              = 0.0f;
    float                       _z              = 0.0f;
    float                       _yaw            = 0.0f;
    float                       _pitch          = 0.0f;

    // 마지막 입력 상태
    int32_t                    _moveFlags      = static_cast< int32_t >( MoveFlag::NONE );

    // 연결된 세션
    RIOSession*                 _session        = nullptr;
};