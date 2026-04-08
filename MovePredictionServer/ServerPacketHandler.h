#pragma once

#include "ServerPacketDispatcher.h"


class GameServer;

// idl 만들면 나중에 자동생성

// 핸들러 등록
void							registerPacketHandlers2( ServerPacketDispatcher& dispatcher ) noexcept;

// 핸들러 함수들
void							handlePlayerJoinReq2( RIOSession* session, void* context, const Packet& packet ) noexcept;
void							handlePlayerLeaveReq2( RIOSession* session, void* context, const Packet& packet ) noexcept;
void							handlePlayerInputReq2( RIOSession* session, void* context, const Packet& packet ) noexcept;
