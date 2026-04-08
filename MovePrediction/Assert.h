#pragma once

#if !defined( NDEBUG )
    #define HDASSERT( expr, fmt, ... ) \
        if ( false == reportAssertImpl( __FILE__, __LINE__, !!( expr ), ( fmt ), ##__VA_ARGS__ ) ) \
            DebugBreak();                                                                           
#else
    #define HDASSERT( expr, fmt, ... ) ( ( void )0 )
#endif


bool reportAssertImpl( const char* file, int line, bool expression, const char* fmt, ... ) noexcept;
