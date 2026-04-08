#include "pch.h"
#include "../MovePrediction/Common.h"
#include "../MovePrediction/LinearAllocator.h"
#include "../MovePrediction/Player.h"
#include "../MovePrediction/Timer.h"
#include "../MovePrediction/PacketHeader.h"
#include "../MovePrediction/Packet.h"
#include "../MovePrediction/PacketUtil.h"

#include "Graphics.h"
#include "NetworkClient.h"
#include "ClientPacketDispatcher.h"
#include "ClientContext.h"


struct Input
{
	bool					_keys[ 256 ];
	int32_t					_mouseX						= 0;
	int32_t					_mouseY						= 0;

	int32_t					_mouseDeltaX				= 0;
	int32_t					_mouseDeltaY				= 0;

	bool					_hasFocus					= false;
};

struct ClientGlobal
{
	Input					_input;
};

struct PredictedMove
{
	float					_dt							= 0.0f;
	PlayerInput				_playerInput;
};

struct PredictedMoveResult
{
	PlayerSnapshotState		_snapshotState;
	PlayerExtraState		_extraState;
};

static ClientGlobal*		getClientGlobal( HWND windowHandle ) noexcept
{
	return ( ClientGlobal* )::GetWindowLongPtr( windowHandle, 0 );
}


LRESULT CALLBACK windowCallback( HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam ) noexcept
{
	ClientGlobal* global	= getClientGlobal( windowHandle );
	
	switch ( message )
	{
	case WM_DESTROY:
		{
			::PostQuitMessage( 0 );
		}
		break;
	case WM_KEYDOWN:
		{
			if ( global->_input._hasFocus )
			{
				assert( wParam < 256 );
				global->_input._keys[ wParam ]	= true;
				if ( VK_ESCAPE == wParam )
				{
					ShowCursor( true );
					global->_input._hasFocus	= false;
				}
			}
		}
		break;
	case WM_KEYUP:
		{
			if ( true == global->_input._hasFocus )
			{
				assert( wParam < 256 );
				global->_input._keys[ wParam ]	= false;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			global->_input._keys[ VK_LBUTTON ]	= true;
			if ( false == global->_input._hasFocus )
			{
				ShowCursor( false );
			}

			global->_input._hasFocus	= true;
		}
		break;
	case WM_LBUTTONUP:
		{
			global->_input._keys[ VK_LBUTTON ]	= false;
		}
		break;
	case WM_RBUTTONDOWN:
		{
			global->_input._keys[ VK_RBUTTON ]	= true;
		}
		break;
	case WM_RBUTTONUP:
		{
			global->_input._keys[ VK_RBUTTON ]	= false;
		}
		break;
	case WM_MOUSEMOVE:
		{
			global->_input._mouseX		= lParam & 0xffff;
			global->_input._mouseY		= ( lParam  >> 16 ) & 0xffff;

			if ( true == global->_input._hasFocus )
			{
				RECT windowRect;
				::GetWindowRect( windowHandle, &windowRect );

				int32_t midX			= ( windowRect.right - windowRect.left ) / 2;
				int32_t midY			= ( windowRect.bottom - windowRect.top ) / 2;

				global->_input._mouseDeltaX		+= midX - global->_input._mouseX;
				global->_input._mouseDeltaY		+= midY - global->_input._mouseY;

				POINT cursorPoint;
				cursorPoint.x			= midX;
				cursorPoint.y			= midY;
				ClientToScreen( windowHandle, &cursorPoint );
				SetCursorPos( cursorPoint.x, cursorPoint.y );
			}
		}
		break;
	default:
		{
			return ::DefWindowProc( windowHandle, message, wParam, lParam );
		}
		break;
	}

	return 0;
}

int CALLBACK WinMain( HINSTANCE instance, HINSTANCE /*hInstance*/, LPSTR /*cmdLine*/, int cmdShow )
{
	WNDCLASS windowClass;
	windowClass.style					= 0;
	windowClass.lpfnWndProc				= windowCallback;
	windowClass.cbClsExtra				= 0;
	windowClass.cbWndExtra				= sizeof( ClientGlobal* );
	windowClass.hInstance				= instance;
	windowClass.hIcon					= 0;
	windowClass.hCursor					= 0;
	windowClass.hbrBackground			= 0;
	windowClass.lpszMenuName			= 0;

	char className[ 64 ];
	sprintf_s( className, "Hodong Window %d", GetCurrentProcessId() );

	windowClass.lpszClassName			= className;//"Hodong Window";

	ATOM windowClassAtom				= ::RegisterClass( &windowClass );
	assert( windowClassAtom );

	constexpr int32_t windowWidth		= 1280;
	constexpr int32_t windowHeight		= 720;

	HWND windowHandle;
	{
		LPCSTR windowName				= className;//"Hodong Window";
		DWORD style						= WS_OVERLAPPED;
		int32_t x						= CW_USEDEFAULT;
		int32_t y						= 0;
		HWND parentWindow				= 0;
		HMENU menu						= 0;
		LPVOID param					= nullptr;

		windowHandle					= CreateWindowA( windowClass.lpszClassName, windowName, style, x, y, windowWidth, windowHeight, parentWindow, menu, instance, param );
		assert( windowHandle );
	}
	
	ShowWindow( windowHandle, cmdShow );

	int32_t sleepGranularityMs			= 1;
	bool sleepGranularityWasSet			= TIMERR_NOERROR == timeBeginPeriod( sleepGranularityMs );

	LinearAllocator allocator( megabytes( 64 ) );
	LinearAllocator tempAllocator( megabytes( 64 ) );

	ClientGlobal* clientGlobal			= ( ClientGlobal* )malloc( sizeof( ClientGlobal ) );
	clientGlobal->_input				= {};

	::SetWindowLongPtr( windowHandle, 0, ( LONG_PTR )clientGlobal );

	GrpahicsState* graphisState			= ( GrpahicsState* )malloc( sizeof( GrpahicsState ) );
	new( graphisState ) GrpahicsState();

	if ( false == graphisState->initialize( windowHandle, instance, windowWidth, windowHeight, 32, &allocator, &tempAllocator ) )
	{
		return false;
	}

	PlayerSnapshotState* playerSnapshotStates	= ( PlayerSnapshotState* )malloc( sizeof( PlayerSnapshotState ) * MAX_CLIENTS );
	bool* playersPresent						= ( bool* )malloc( sizeof( bool ) * MAX_CLIENTS );
	memset( playersPresent, 0, sizeof( bool ) * MAX_CLIENTS );  // 추가
	Matrix4x4* mvpMatrices						= ( Matrix4x4* )malloc( sizeof( Matrix4x4 ) * ( MAX_CLIENTS + 1 ) );

	PlayerSnapshotState* localPlayerSnapshotState	= ( PlayerSnapshotState* )malloc( sizeof( PlayerSnapshotState ) );
	PlayerExtraState* localPlayerExtraState		= ( PlayerExtraState* )malloc( sizeof( PlayerExtraState ) );

	*localPlayerSnapshotState					= {};
	*localPlayerExtraState						= {};

	constexpr int32_t predictionBufferCapacity	= 512;
	constexpr int32_t predictionBufferMask		= predictionBufferCapacity - 1;

	PredictedMove* predictedMove				= ( PredictedMove* )malloc( sizeof( PredictedMove ) * predictionBufferCapacity );
	PredictedMoveResult* predictedMoveResult	= ( PredictedMoveResult* )malloc( sizeof( PredictedMoveResult ) * predictionBufferCapacity );

	Vector3* playerTargetPositions				= ( Vector3* )malloc( sizeof( Vector3 ) * MAX_CLIENTS );
	memset( playerTargetPositions, 0, sizeof( Vector3 ) * MAX_CLIENTS );

	constexpr float fovY						= 60.0f * DegToRad;
	constexpr float aspectRatio					= windowWidth / ( float )windowHeight;
	constexpr float nearPlane					= 1.0f;
	constexpr float farPlane					= 100.0f;

	Matrix4x4 projectionMatrix					= {};
	projectionMatrix.projection( fovY, aspectRatio, nearPlane, farPlane );

	constexpr int32_t tickRate					= 60;
	constexpr float secondPerTick				= 1.0f / tickRate;

	//int32_t localPlayerSlot						= -1;
	int32_t localPlayerSlot						= 0;
	int32_t predictionId						= 0;

	Timer tickTimer								= Timer();

	int32_t exitCode							= 0;

	// network test
	//{
		WSAData wsaData = {};
		if ( 0 != ::WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) )
		{
			std::cout << "WSAStartup failed" << std::endl;
			return 1;
		}

		NetworkClient client;

		if ( !client.connect( "127.0.0.1", 5000 ) )
		{
			std::cout << "connect failed" << std::endl;
			::WSACleanup();
			return 1;
		}

		std::cout << "connected to server!" << std::endl;

		ClientContext clientContext             = {};
		clientContext._localPlayerSnapshotState		= localPlayerSnapshotState;
		clientContext._localPlayerExtraState	= localPlayerExtraState;
		clientContext._playerSnapshotStates     = playerSnapshotStates;
		clientContext._playerPresents			= playersPresent;
		clientContext._playerTargetPositions	= playerTargetPositions;

		// dispatcher 생성 및 핸들러 등록
		ClientPacketDispatcher dispatcher;
		dispatcher.setContext( &clientContext );
		registerClientHandlers( dispatcher );

		// 테스트: 에코 패킷 전송
		// PacketHeader(4) + payload
		PlayerJoinReq joinReq					= {};
		strcpy_s( joinReq._name, "TestPlayer" );

		auto joinPacket = buildPacket( PacketId::PLAYER_JOIN_REQ, joinReq );
		client.send( joinPacket.data(), static_cast<int32_t>( joinPacket.size() ) );
		std::cout << "Sent PLAYER_JOIN_REQ" << std::endl;

	//}


	while ( true )
	{
		bool gotQuitMessage						= false;
		MSG message;
		HWND hwnd								= NULL;
		uint32_t filterMin						= 0;
		uint32_t filterMax						= 0;
		uint32_t removeMessage					= PM_REMOVE;
		while ( PeekMessage( &message, hwnd, filterMin, filterMax, removeMessage ) )
		{
			if ( WM_QUIT == message.message )
			{
				exitCode						= ( int32_t )message.wParam;
				gotQuitMessage					= true;
				break;
			}

			::TranslateMessage( &message );
			::DispatchMessage( &message );
		}

		if ( true == gotQuitMessage )
		{
			break;
		}

		// network test
		Packet recvPacket;
        while ( client.tryGetPacket( recvPacket ) )
        {
            if ( !dispatcher.dispatch( recvPacket ) )
            {
                std::cout << "[WARN] Unknown packet: " << recvPacket._id << std::endl;
            }
        }

		int32_t mouseDeltaX						= clientGlobal->_input._mouseDeltaX;
		int32_t mouseDeltaY						= clientGlobal->_input._mouseDeltaY;

		clientGlobal->_input._mouseDeltaX		= 0;
		clientGlobal->_input._mouseDeltaY		= 0;

		// network receive
		{
		
		}

		// tick local player
		if ( -1 != localPlayerSlot )
		{
			constexpr float mouseSensitivity	= 0.003f;

			PlayerInput playerInput				= {};
			playerInput._left					= clientGlobal->_input._keys[ 'A' ];
			playerInput._right					= clientGlobal->_input._keys[ 'D' ];
			playerInput._up						= clientGlobal->_input._keys[ 'W' ];
			playerInput._down					= clientGlobal->_input._keys[ 'S' ];
			playerInput._jump					= clientGlobal->_input._keys[ VK_SPACE ];
			playerInput._pitch					= std::clamp( localPlayerSnapshotState->_pitch - ( mouseDeltaY * mouseSensitivity ), -85.0f * DegToRad, 85.0f * DegToRad );
			playerInput._yaw					= localPlayerSnapshotState->_yaw + ( mouseDeltaX * mouseSensitivity );

			float dt							= secondPerTick;

			// ========== 서버에 입력 전송 ==========
			{
				static PlayerInput lastSentInput = {};
				static float sendTimer			= 0.0f;
				constexpr float SEND_INTERVAL	= 0.1f;  // 100ms


				sendTimer						+= dt;

				// 입력 변경 또는 100ms마다 전송
				bool inputChanged               = ( playerInput._up != lastSentInput._up ) || ( playerInput._down != lastSentInput._down ) || ( playerInput._left != lastSentInput._left ) || ( playerInput._right != lastSentInput._right );
				if ( true == inputChanged || SEND_INTERVAL <= sendTimer)
				{
					PlayerInputReq inputReq     = {};
					inputReq._moveFlag			= 0;

					if ( true == playerInput._up )
					{
						inputReq._moveFlag      |= static_cast< int32_t >( MoveFlag::UP );
					}

					if ( true == playerInput._down )
					{
						inputReq._moveFlag      |= static_cast< int32_t >( MoveFlag::DOWN );
					}

					if ( true == playerInput._left )
					{
						inputReq._moveFlag      |= static_cast< int32_t >( MoveFlag::LEFT );
					}

					if ( true == playerInput._right )
					{
						inputReq._moveFlag      |= static_cast< int32_t >( MoveFlag::RIGHT );
					}

					inputReq._yaw               = playerInput._yaw;
					inputReq._pitch             = playerInput._pitch;

					std::vector< char> inputPacket = buildPacket( PacketId::PLAYER_INPUT_REQ, inputReq );
					client.send( inputPacket.data(), static_cast< int32_t >( inputPacket.size() ) );

					lastSentInput               = playerInput;
					sendTimer                   = 0.0f;
				}
			}

			localPlayerSnapshotState->_pitch = playerInput._pitch;
			localPlayerSnapshotState->_yaw = playerInput._yaw;

			tickPlayer( localPlayerSnapshotState, localPlayerExtraState, dt, &playerInput );

			// 서버 보정 보간 player
			if ( true == clientContext._hasServerTarget )
			{
				constexpr float lerpFactor                  = 0.2f;
    
				localPlayerSnapshotState->_position._x      += ( clientContext._serverTargetPosition._x - localPlayerSnapshotState->_position._x ) * lerpFactor;
				localPlayerSnapshotState->_position._y      += ( clientContext._serverTargetPosition._y - localPlayerSnapshotState->_position._y ) * lerpFactor;
				localPlayerSnapshotState->_position._z      += ( clientContext._serverTargetPosition._z - localPlayerSnapshotState->_position._z ) * lerpFactor;
			}

			// 다른 플레이어 보간
			constexpr float otherLerpFactor = 0.2f;
			for ( int32_t ii = 1; ii < MAX_CLIENTS; ++ii )
			{
				if ( true == playersPresent[ ii ] )
				{
					playerSnapshotStates[ ii ]._position._x += ( clientContext._playerTargetPositions[ ii]._x - playerSnapshotStates[ ii ]._position._x ) * otherLerpFactor;
					playerSnapshotStates[ ii ]._position._y += ( clientContext._playerTargetPositions[ ii]._y - playerSnapshotStates[ ii ]._position._y ) * otherLerpFactor;
					playerSnapshotStates[ ii ]._position._z += ( clientContext._playerTargetPositions[ ii]._z - playerSnapshotStates[ ii ]._position._z ) * otherLerpFactor;
				}
			}

			int32_t index						= predictionId & predictionBufferMask;

			PredictedMove* move					= &predictedMove[ index ];
			PredictedMoveResult* moveResult		= &predictedMoveResult[ index ];

			move->_dt							= dt;
			move->_playerInput					= playerInput;
			moveResult->_snapshotState			= *localPlayerSnapshotState;
			moveResult->_extraState				= *localPlayerExtraState;

			playerSnapshotStates[ localPlayerSlot ]	= *localPlayerSnapshotState;
			predictionId						+= 1;
		}

		constexpr float cameraOffsetDistance	= 3.0f;
		Vector3 cameraPosition					= localPlayerSnapshotState->_position;
		cameraPosition._z						+= 1.8f;
		cameraPosition._y						-= 5.8f;

		Quaternion cameraRotation				= Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
		cameraRotation.angleAxis( Vector3( 0.0f, 0.0f, 1.0f ), localPlayerSnapshotState->_yaw );

		Quaternion tempRotation					= Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
		tempRotation.angleAxis( Vector3( 1.0f, 0.0f, 0.0f ), localPlayerSnapshotState->_pitch );

		cameraRotation.mul( tempRotation );

		Matrix4x4 viewMatrix					= {};
		viewMatrix.camera( cameraPosition, cameraRotation.right(), cameraRotation.forward(), cameraRotation.up() );

		Matrix4x4 viewProjectionMatrix			= projectionMatrix;
		viewProjectionMatrix.mul( viewMatrix );

		mvpMatrices[ 0 ]						= viewProjectionMatrix;

		Matrix4x4 templTranslationMatrix;
		Matrix4x4 tempRotationMatrix;
		Matrix4x4 tempModelMatrix;

		bool* playersPresentEnd					= &playersPresent[ MAX_CLIENTS ];
		PlayerSnapshotState* playSnapshotState	= &playerSnapshotStates[ 0 ];
		Matrix4x4* playerMatrix					= &mvpMatrices[ 1 ];
		playersPresent[ 0 ]						= true;

		for ( bool* playerPresent = &playersPresent[ 0 ]; playerPresent != playersPresentEnd; ++playerPresent, ++playSnapshotState )
		{
			if ( true == *playerPresent )
			{
				tempRotationMatrix.rotationZ( playSnapshotState->_yaw );
				templTranslationMatrix.translation( playSnapshotState->_position );

				tempModelMatrix					= templTranslationMatrix;
				tempModelMatrix.mul( tempRotationMatrix );
				
				*playerMatrix					= viewProjectionMatrix;
				playerMatrix->mul( tempModelMatrix );

				playerMatrix					+= 1;
			}
		}

		int32_t numMatrix						= playerMatrix - &mvpMatrices[ 1 ];
		graphisState->updateAndDraw( mvpMatrices, numMatrix );

		tickTimer.waitUntil( secondPerTick, sleepGranularityWasSet );
		tickTimer.shiftStart( secondPerTick );
	}

	return 0;
}