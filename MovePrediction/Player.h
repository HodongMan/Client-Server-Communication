#pragma once


#include "Math.h"


struct PlayerInput
{
	bool						_up				= false;
	bool						_down			= false;
	bool						_left			= false;
	bool						_right			= false;
	bool						_jump			= false;

	float						_pitch			= 0.0f;
	float						_yaw			= 0.0f;
};

struct PlayerSnapshotState
{
	Vector3						_position		= Vector3( 0.0f, 0.0f, 0.0f );
	float						_pitch			= 0.0f;
	float						_yaw			= 0.0f;
};

struct PlayerExtraState
{
	Vector3						_velocity		= Vector3( 0.0f, 0.0f, 0.0f );
};


void							tickPlayer( PlayerSnapshotState* playerSnapshotState, PlayerExtraState* playerExtraState, float dt, PlayerInput* playerInput ) noexcept;