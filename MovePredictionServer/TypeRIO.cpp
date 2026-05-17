#include "pch.h"
#include "TypeRIO.h"

#include "RIOContext.h"
#include "RIOSession.h"
#include "RIOManager.h"


void releaseRIOContext( RIOContext* context ) noexcept
{
	HDASSERT( nullptr != context, "Context 정보가 비정상 입니다." );

	_rioManager->getContextPool().release( context );
}