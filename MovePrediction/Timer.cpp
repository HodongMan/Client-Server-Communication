#include "CommonPch.h"
#include "Timer.h"


Timer::Timer( void ) noexcept
{
	::QueryPerformanceFrequency( &_frequency );
	::QueryPerformanceCounter( &_start );
}

float Timer::getSecond( void ) const noexcept
{
	LARGE_INTEGER now;
	::QueryPerformanceCounter( &now );

	return ( float )( now.QuadPart - _start.QuadPart ) / ( float )_frequency.QuadPart;
}

void Timer::waitUntil( float waitTimeSecond, bool isSleepGranularity ) noexcept
{
	float timeTakenSeconds					= getSecond();

	while ( timeTakenSeconds < waitTimeSecond )
	{
		if ( true == isSleepGranularity )
		{
			DWORD timeToWaitMillisecond		= ( DWORD )( waitTimeSecond - timeTakenSeconds ) * 1000;
			if ( 1 < timeToWaitMillisecond )
			{
				Sleep( timeToWaitMillisecond );
			}
		}

		timeTakenSeconds					= getSecond();
	}
}

void Timer::shiftStart( float accumulateSecond ) noexcept
{
	_start.QuadPart							+= ( LONGLONG )( _frequency.QuadPart * accumulateSecond );
}
