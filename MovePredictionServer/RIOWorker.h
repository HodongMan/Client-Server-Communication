#pragma once



class RIOWorker
{
public:
	RIOWorker( void ) noexcept;
	~RIOWorker( void ) noexcept;

	bool											start( int32_t threadIndex ) noexcept;

	
	RIO_CQ											getCompletionQueue( void ) const noexcept;

private:
	static unsigned int WINAPI						doIOWorkerThread( LPVOID lpParam ) noexcept;

private:
	RIO_CQ											_completionQueue				= RIO_INVALID_CQ;
	int32_t											_threadIndex					= 0;
	OVERLAPPED										_overlapped						= {};
	HANDLE											_threadHandle					= INVALID_HANDLE_VALUE;
};