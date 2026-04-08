#pragma once


struct Timer
{
	LARGE_INTEGER				_start;
	LARGE_INTEGER				_frequency;

	Timer( void ) noexcept;

	float						getSecond( void ) const noexcept;
	void						waitUntil( float waitTimeSecond, bool isSleepGranularity ) noexcept;
	void						shiftStart( float accumulateSecond ) noexcept;
};