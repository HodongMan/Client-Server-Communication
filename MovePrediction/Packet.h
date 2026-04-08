#pragma once


#include "CommonPch.h"

enum class MoveFlag
{
    NONE,
    UP                          = 1 << 0,  // W
    DOWN                        = 1 << 1,  // S
    LEFT                        = 1 << 2,  // A
    RIGHT                       = 1 << 3,  // D
    MAX
};


#pragma pack( push, 1 )



struct PlayerJoinReq
{
    char                        _name[ 32 ];
};

struct PlayerJoinRes
{
    int32_t                     _playerId               = 0;
};

struct PlayerLeaveReq
{
    int32_t                     _playerId               = 0;
};

struct PlayerLeaveRes
{
    int32_t                     _playerId               = 0;
    bool                        _isSuccess              = false;
};

struct PlayerInputReq
{
    int32_t                     _moveFlag               = 0;
    float                       _pitch                  = 0.f;
    float                       _yaw                    = 0.f;
};

struct PlayerStateRes
{
    int32_t                     _playerId               = 0;
    float                       _x                      = 0.f;
    float                       _y                      = 0.f;
    float                       _z                      = 0.f;
    float                       _pitch                  = 0.f;
    float                       _yaw                    = 0.f;
};

#pragma pack( pop )
