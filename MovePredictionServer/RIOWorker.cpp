#include "pch.h"
#include "RIOWorker.h"

#include "RIOContext.h"
#include "RIOSession.h"
#include "RIOManager.h"


RIOWorker::RIOWorker( void ) noexcept
{

}

RIOWorker::~RIOWorker( void ) noexcept
{
	HDASSERT( RIO_INVALID_CQ != _completionQueue, "Completion Queue가 반드시 초기화 되었어야 합니다." );
	_rioManager->_rioFunctionTable.RIOCloseCompletionQueue( _completionQueue );

	_endthreadex( 0 );
}

bool RIOWorker::start( int32_t threadIndex ) noexcept
{
	HDASSERT( 0 <= threadIndex, "Thread Index는 0부터 시작하니까!" );
	HDASSERT( INVALID_HANDLE_VALUE != _rioManager->getIocpHandle(), "IOCP Handle이 먼저 있어야 합니다." );

	// create completion queue joined thread
	RIO_NOTIFICATION_COMPLETION notification	= {};
	notification.Type					= RIO_IOCP_COMPLETION;
	notification.Iocp.IocpHandle		= _rioManager->getIocpHandle();
	notification.Iocp.CompletionKey		= ( PVOID )RIO_COMPLETION_KEY;
	notification.Iocp.Overlapped		= this;

	_completionQueue					= _rioManager->_rioFunctionTable.RIOCreateCompletionQueue( MAX_CQ_SIZE, &notification );
	if ( RIO_INVALID_CQ == _completionQueue )
	{
		Logger::log( LogLevel::ERRORS, "%s error: RIOCreateCompletionQueue error\n", __FUNCTION__ );
		return false;
	}

	if ( ERROR_SUCCESS != _rioManager->_rioFunctionTable.RIONotify( _completionQueue ) )
	{
		Logger::log( LogLevel::ERRORS, "%s error: RIONotify error\n", __FUNCTION__ );
		return false;
	}

	_threadIndex						= threadIndex;

	DWORD threadId						= 0;
	
	HANDLE threadHandle					= ( HANDLE )_beginthreadex( NULL, 0, doIOWorkerThread, ( LPVOID )_rioManager->getIocpHandle(), 0, (unsigned int*)&threadId );
	if ( INVALID_HANDLE_VALUE == threadHandle )
	{
		Logger::log( LogLevel::ERRORS, "%s error: _beginthreadex error\n", __FUNCTION__ );
		return false;
	}

	_threadHandle						= threadHandle;

	const std::wstring threadName		= L"IO Thread " + std::to_wstring( threadIndex + 1 );

	HRESULT hr							= ::SetThreadDescription( threadHandle, threadName.c_str() );
	if ( FAILED( hr ) )
	{
		// 이름 적는거 실패한다고 실패처리할 필요는...
		HDASSERT( false, "%s error: SetThreadDescription 함수의 실패!", __FUNCTION__ );
	}

	return true;
}

RIO_CQ RIOWorker::getCompletionQueue( void ) const noexcept
{
	HDASSERT( RIO_INVALID_CQ != _completionQueue, "completion queue가 생성이 된 다음에 호출해야 합니다 start를 먼저해주세요" );
	return _completionQueue;
}

unsigned int __stdcall RIOWorker::doIOWorkerThread( LPVOID lpParam ) noexcept
{
	HANDLE iocpHandle				= ( HANDLE )lpParam;
	DWORD numberOfBytes				= 0;
	ULONG_PTR completionKey			= 0;
	OVERLAPPED* overlapped			= nullptr;

	RIORESULT results[ MAX_RIO_RESULT ];

	while ( true )
	{
		BOOL result					= ::GetQueuedCompletionStatus( iocpHandle, &numberOfBytes, &completionKey, &overlapped, INFINITE );
		if ( false == result )
		{
			Logger::log( LogLevel::ERRORS, "%s error: GetQueuedCompletionStatus error : %d \n", __FUNCTION__, ::GetLastError() );
			break;
		}

		if ( RIO_COMPLETION_KEY != completionKey && nullptr != overlapped )
		{
			OverlappedContext* overlappedContext	= ( OverlappedContext* )overlapped;
			if ( IOType::ACCEPT == overlappedContext->_ioType )
			{
				overlappedContext->_session->acceptCompletion();
				delete static_cast< OverlappedAcceptContext* >( overlappedContext );
			}
			else if ( IOType::DISCONNECT == overlappedContext->_ioType )
			{
				overlappedContext->_session->disconnectCompletion();
				delete static_cast< OverlappedDisconnectContext* >( overlappedContext );
			}
			else
			{
				HDASSERT( false, "IOCP 처리는 오직 Accept과 Disconnect만 처리 합니다 다른 처리는 여기서 하면 안됩니다." );
			}

			continue;
		}

		RIOWorker* worker			= reinterpret_cast< RIOWorker* >( overlapped );
		HDASSERT( nullptr != worker, "이것이 invalid 할 리가 뭔가 잘못되었습니다." );

		ZeroMemory( results, sizeof( results ) );
		ULONG numResults			= _rioManager->_rioFunctionTable.RIODequeueCompletion( worker->_completionQueue, results, MAX_RIO_RESULT );
		
		if ( 0 == numResults )
		{
			Logger::log( LogLevel::ERRORS, "%s error: numResults 0 error \n", __FUNCTION__ );
			break;
		}

		if ( RIO_CORRUPT_CQ == numResults )
		{
			Logger::log( LogLevel::ERRORS, "%s error: RIONotify RIO_CORRUPT_CQ error \n", __FUNCTION__ );
			break;
		}

		int32_t notifyResult		= _rioManager->_rioFunctionTable.RIONotify( worker->_completionQueue );
		if ( ERROR_SUCCESS != notifyResult )
		{
			Logger::log( LogLevel::ERRORS, "%s error: RIONotify error \n", __FUNCTION__ );
			
			break;
		}

		for ( ULONG ii = 0; ii < numResults; ++ii )
		{
			RIOContext* context		= reinterpret_cast< RIOContext* >( results[ ii ].RequestContext );
			HDASSERT( nullptr != context, "RIOContext 데이터가 비정상 입니다. 구조 변경이 있었나요?" );

			RIOSession* session		= context->_session;
			HDASSERT( nullptr != session, "RIOSession 데이터가 비정상 입니다. 구조 변경이 있었나요?" );

			ULONG transferred		= results[ ii ].BytesTransferred;
			if ( 0 == transferred )
			{
				session->requestDisconnect( DisconnectReason::ACTIVE );
				releaseRIOContext( context );
				
				continue;
			}

			switch ( context->_ioType )
			{
			case IOType::RECV:
				{
					session->recvCompletion( transferred );
					releaseRIOContext( context );
				}
				break;
			case IOType::SEND:
				{
					session->sendCompletion( transferred );
					releaseRIOContext( context );
				}
				break;
			case IOType::ACCEPT:
			case IOType::DISCONNECT:
				{
					HDASSERT( false, "해당 로직은 RIO 처리에서 하는 것이 아닙니다. 매우 비정상!!" );
				}
				break;
			default:
				{
					HDASSERT( false, "처리가 추가되면 이 곳에서도 작업을 해야 합니다." );
				}
				break;
			}

			/*
			if ( IOType::RECV == context->_ioType )
			{
				session->recvCompletion( transferred );
				if ( false == session->postSend( transferred ) )
				{
					session->requestDisconnect( DisconnectReason::ACTIVE );
				}
			}
			else if ( IOType::SEND == context->_ioType )
			{
				session->sendCompletion( transferred );
				if ( false == session->postRecv() )
				{
					session->requestDisconnect( DisconnectReason::ACTIVE );
				}
			}
			else
			{
				HDASSERT( false, "다른 type을 추가 했으면 여기도 확인해야 합니다." );
			}
			*/
		}
	}

	return 0;
}