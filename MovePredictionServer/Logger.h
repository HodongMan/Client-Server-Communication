#pragma once


enum class LogLevel
{
	DEBUG,
	WARD,
	ERRORS,
};

class Logger
{
public:
	template< typename ... Args >
	static void						log( LogLevel logLevel, Args ... args ) noexcept
	{
		if ( logLevel <= _logLevel )
		{
			std::printf( args ... );
			std::fflush( stdout );
		}
	}

	static void						setLogLevel( LogLevel logLevel ) noexcept
	{
		_logLevel					= logLevel;
	}


private:
	static LogLevel					_logLevel;
};