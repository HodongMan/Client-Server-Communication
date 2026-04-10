#include "pch.h"
#include "ConditionVariableSRW.h"

#include "RWLock.h"


ConditionVariableSRW::ConditionVariableSRW( RWLock* locker ) noexcept
	: _locker{ locker }
{
	assert( _locker );
	::InitializeConditionVariable( &_conditionVariable );
}

void ConditionVariableSRW::sleepConditionVariable( void ) noexcept
{
	::SleepConditionVariableSRW( &_conditionVariable, &_locker->_locker, INFINITE, 0 );
}

void ConditionVariableSRW::wakeOne( void ) noexcept
{
	::WakeConditionVariable( &_conditionVariable );
}

void ConditionVariableSRW::wakeAll( void ) noexcept
{
	::WakeAllConditionVariable( &_conditionVariable );
}
