#pragma once


#include "RIOContext.h"
#include "RWLock.h"


class SyncContextPool
{
public:
	SyncContextPool( void ) noexcept = default;
	~SyncContextPool( void ) noexcept;

	SyncContextPool( const SyncContextPool& ) noexcept = delete;
	SyncContextPool&										operator=( const SyncContextPool& ) noexcept = delete;

	bool													initialize( int32_t capacity ) noexcept;
	void													shutdown( void ) noexcept;

	RIOContext*												acquire( RIOSession* session, IOType ioType ) noexcept;
	void													release( RIOContext* context ) noexcept;

private:
	std::vector< RIOContext* >								_pool;
	std::vector< RIOContext* >								_freelist;

	RWLock													_lock;
	bool													_initialized				= false;
};