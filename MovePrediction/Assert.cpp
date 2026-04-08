#include "CommonPch.h"
#include "Assert.h"


bool reportAssertImpl( const char* file, int line, bool expression, const char* format, ... ) noexcept
{
	if ( expression )
	{
		return true;
	}

	char message[ 512 ]						= {};
    va_list ap;
    va_start( ap, format );
    vsnprintf_s( message, _TRUNCATE, format, ap );
    va_end( ap );

    char fullMessage[ 768 ]					= {};
    _snprintf_s( fullMessage, _TRUNCATE, "FILE %s : LINE %d ERROR : %s\n", file, line, message );

    OutputDebugStringA( fullMessage );
    std::fputs( fullMessage, stderr );

    int32_t result							= ::MessageBoxA( nullptr, fullMessage, "HDEngine Error", MB_ABORTRETRYIGNORE | MB_ICONERROR );

	switch ( result )
	{
	case IDABORT:
		{
			assert( !"HDASSERT Triggered Abort" );
			return false;
		}
		break;
	case IDRETRY:
		{
			return false;
		}
		break;
	case IDIGNORE:
		{
			return true;
		}
		break;
	default:
		break;
	};

	return false;
}
