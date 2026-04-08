#pragma once

#include "pch.h"
#include "../MovePrediction/Player.h"


struct ClientContext
{
	int32_t							_playerId						= 0;
	PlayerSnapshotState*			_localPlayerSnapshotState		= nullptr;
	PlayerExtraState*				_localPlayerExtraState			= nullptr;

	// Server Status
	Vector3							_serverTargetPosition			= Vector3( 0.0f, 0.0f, 0.0f );
	bool							_hasServerTarget				= true;

	// near other player
	PlayerSnapshotState*					_playerSnapshotStates	= nullptr;
	bool*									_playerPresents			= nullptr;
	std::unordered_map< int32_t, int32_t >	_playerIdToSlot;

	// 다른 플레이어 타겟 위치 (보간용)
	Vector3* _playerTargetPositions									= nullptr;

	int32_t									_nextSlot				= 1;
};