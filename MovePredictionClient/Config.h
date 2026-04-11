#pragma once


#include "pch.h"
#include "../MovePrediction/Math.h"


// Window
constexpr int32_t				WINDOW_WIDTH				= 1280;
constexpr int32_t				WINDOW_HEIGHT				= 720;

// Tick
constexpr int32_t				TICK_RATE					= 60;
constexpr float					SECOND_PER_TICK				= 1.0f / TICK_RATE;

// Camera
constexpr float					FOV_Y						= 60.0f * DegToRad;  // DegToRad
constexpr float					NEAR_PLANE					= 1.0f;
constexpr float					FAR_PLANE					= 100.0f;
constexpr float					MOUSE_SENSITIVITY			= 0.003f;

// Network
constexpr float					SEND_INTERVAL				= 0.1f;

// Interpolation
constexpr float					LEAP_FACTOR					= 0.2f;
constexpr float					OTHER_LERP_FACTOR			= 0.2f;


constexpr int32_t				MAX_CLIENTS					= 32;
constexpr int32_t				PREDICTION_BUFFER_CAPACITY	= 512;