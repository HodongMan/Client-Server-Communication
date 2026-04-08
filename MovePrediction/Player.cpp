#include "CommonPch.h"
#include "Player.h"


constexpr float							playerMovementSpeed				= 10.0f;
constexpr float							playerMovementSpeedSqaure		= playerMovementSpeed * playerMovementSpeed;
constexpr float							playerJumpingSpeed				= 4.5f;
constexpr float							playerAirControl				= 2.0f;


void tickPlayer( PlayerSnapshotState* playerSnapshotState, PlayerExtraState* playerExtraState, float dt, PlayerInput* playerInput ) noexcept
{
	HDASSERT( nullptr != playerSnapshotState, "PlayerSnapshotState의 데이터가 비정상 입니다." );
	HDASSERT( nullptr != playerExtraState, "PlayerExtraState의 데이터가 비정상 입니다." );
	HDASSERT( nullptr != playerInput, "PlayerInput의 데이터가 비정상 입니다." );

	float cosYaw						= cosf( playerInput->_yaw );
	float sinYaw						= sinf( playerInput->_yaw );

	Vector3 right						= Vector3( cosYaw, sinYaw, 0.0f );
	Vector3 forward						= Vector3( -sinYaw, cosYaw, 0.0f );

	Vector3 desiredMovemoentDirection	= Vector3( 0.0f, 0.0f, 0.0f );
	if ( true == playerInput->_up )
	{
		desiredMovemoentDirection.add( forward );
	}

	if ( true == playerInput->_down )
	{
		desiredMovemoentDirection.sub( forward );
	}

	if ( true == playerInput->_left )
	{
		desiredMovemoentDirection.sub( right );
	}

	if ( true == playerInput->_right )
	{
		desiredMovemoentDirection.add( right );
	}

	desiredMovemoentDirection.normalize();

	Vector3 velocity( 0.0f, 0.0f, 0.0f );

	bool isGrounded					= 0.0f == playerSnapshotState->_position._z;
	if ( true == isGrounded )
	{
		velocity						= desiredMovemoentDirection;
		velocity.mul( playerMovementSpeed );

		if ( true == playerInput->_jump )
		{
			velocity._z					+= playerJumpingSpeed;
			isGrounded					= false;
		}
	}
	else
	{
		velocity						= playerExtraState->_velocity;
	}

	playerSnapshotState->_pitch			= playerInput->_pitch;
	playerSnapshotState->_yaw			= playerInput->_yaw;

	if ( false == isGrounded )
	{
		Vector3 gravity					= Vector3( 0.0f, 0.0f, -9.81f );
		Vector3 airControl				= desiredMovemoentDirection;
		airControl.mul( playerMovementSpeed * playerAirControl );

		Vector3 acceleration			= gravity;
		acceleration.add( airControl );

		Vector3 velocityDelta			= acceleration;
		velocityDelta.mul( dt );

		Vector3 finalVelocity			= velocity;
		finalVelocity.add( velocityDelta );

		Vector3 finalVelocityXY			= Vector3( finalVelocity._x, finalVelocity._y, 0.0f );
		if ( playerMovementSpeedSqaure < finalVelocityXY.getLength() )
		{
			finalVelocityXY.normalize();
			finalVelocityXY.mul( playerMovementSpeed );

			finalVelocity._x			= finalVelocityXY._x;
			finalVelocity._y			= finalVelocityXY._y;
		}

		// s = (u + v) * 0.5 * t;
		Vector3 positionDelta			= velocity;
		positionDelta.add( finalVelocity );
		positionDelta.mul( 0.5f * dt );

		playerSnapshotState->_position.add( positionDelta );
		playerSnapshotState->_position._z	= std::max< float >( playerSnapshotState->_position._z, 0.0f );

		playerExtraState->_velocity		= finalVelocity;
	}
	else
	{
		Vector3 newVelocity				= velocity;
		newVelocity.mul( dt );
		playerSnapshotState->_position.add( newVelocity );
		playerExtraState->_velocity		= velocity;
	}
}
