#pragma once


class RWLock;


class ConditionVariableSRW
{
public:
	explicit ConditionVariableSRW( RWLock* locker ) noexcept;

	void									sleepConditionVariable( void ) noexcept;
	void									wakeOne( void ) noexcept;
	void									wakeAll( void ) noexcept;

private:
	CONDITION_VARIABLE						_conditionVariable;
	RWLock*									_locker;
};