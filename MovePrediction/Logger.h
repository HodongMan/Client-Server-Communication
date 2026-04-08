#pragma once



void log( const char* format, ... ) noexcept
{
	char buffer[ 512 ];

	va_list args;
	va_start( args, format );
	vsnprintf( buffer, sizeof( buffer ), format, args );
	va_end( args );

	OutputDebugStringA( buffer );
}