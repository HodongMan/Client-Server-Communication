#include "pch.h"
#include "TypeRIO.h"

#include "RIOContext.h"
#include "RIOSession.h"


void releaseRIOContext( RIOContext* context ) noexcept
{
	assert( nullptr != context );
	context->_session->releaseReference();

	delete context;
}