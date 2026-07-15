#include "CommonPch.h"
#include "Lexer.h"


Lexer::Lexer( std::string source ) noexcept
	: _source( std::move( source ) )
{

}

Token Lexer::next( void ) noexcept
{
	if ( true == _hasPeeked )
	{
		_hasPeeked								= false;
		return _peeked;
	}

	skipWhitespaceAndComments();

	if ( true == isEnd() )
	{
		return makeToken( TypeToken::END_OF_FILE, "", _line, _column );
	}

	const char character						= current();
	const int32_t startLine						= _line;
	const int32_t startColumn					= _column;

	if ( true == isIdentifierStart( character ) )
	{
		return readIdentifierOrKeyword();
	}

	if ( true == isDigit( character ) )
	{
		return readNumber();
	}

	switch ( character )
	{
	case '=':
		{
			advance();
			return makeToken( TypeToken::EQUALS, "=", startLine, startColumn );
		}
		break;
	case '{':
		{
			advance();
			return makeToken( TypeToken::LBRACE, "{", startLine, startColumn );
		}
		break;
	case '}':
		{
			advance();
			return makeToken( TypeToken::RBRACE, "}", startLine, startColumn );
		}
		break;
	case ';':
		{
			advance();
			return makeToken( TypeToken::SEMICOLON, ";", startLine, startColumn );
		}
		break;
	default:
		{
	
		}
		break;
	}

	{
		std::string text( 1, character );
		advance();

		return makeToken( TypeToken::UNKNOWN, std::move( text ), startLine, startColumn );
	}
}

Token Lexer::peek( void ) noexcept
{
	if ( false == _hasPeeked )
	{
		_peeked									= next();
		_hasPeeked								= true;
	}

	return _peeked;
}

void Lexer::skipWhitespaceAndComments( void ) noexcept
{
	while ( false == isEnd() )
	{
		// 공백 종류 처리
		const char character					= current();
		if ( ' ' == character || '\t' == character || '\r' == character || '\n' == character )
		{
			advance();
			continue;
		}

		// 주석 처리
		if ( '/' == character && '/' == lookahead() )
		{
			while ( false == isEnd() && '\n' != current() )
			{
				advance();
			}

			continue;
		}

		break;
	}
}

Token Lexer::readIdentifierOrKeyword( void ) noexcept
{
	const int32_t startLine						= _line;
	const int32_t startColumn					= _column;
	const int64_t startPosition					= _position;

	while ( false == isEnd() && isIdentifierBody( current() ) )
	{
		advance();
	}

	std::string text							= _source.substr( startPosition, _position - startPosition );

	if ( "packet" == text )
	{
		return makeToken( TypeToken::PACKET, std::move( text ), startLine, startColumn );
	}

	return makeToken( TypeToken::IDENTIFIER, std::move( text ), startLine, startColumn );
}

Token Lexer::readNumber(void) noexcept
{
	return Token();
}

Token Lexer::makeToken( TypeToken type, std::string text, int32_t line, int32_t column ) const noexcept
{
	HDASSERT( TypeToken::MAX != type, "Type 정보가 비정상 입니다." );
	HDASSERT( false == text.empty(), "Text 정보가 비정상 입니다." );
	HDASSERT( 0 <= line, "Line 정보가 비정상 입니다." );
	HDASSERT( 0 <= column, "Column 정보가 비정상 입니다." );

	Token token									= {};
	token._type									= type;
	token._text									= std::move( text );
	token._line									= line;
	token._column								= column;

	return token;
}

char Lexer::current( void ) const noexcept
{
	if ( _source.size() <= _position )
	{
		return '\0';
	}

	return _source[ _position ];
}

char Lexer::lookahead( void ) const noexcept
{
	if ( _source.size() <= _position + 1 )
	{
		return '\0';
	}

	return _source[ _position + 1 ];
}

bool Lexer::isEnd( void ) const noexcept
{
	return _source.size() <= _position;
}

void Lexer::advance( void ) noexcept
{
	if ( _source.size() <= _position )
	{
		return;
	}

	if ( '\n' == _source[ _position ] )
	{
		_line										+= 1;
		_column										+= 1;
	}
	else
	{
		_column										+= 1;
	}

	_position										+= 1;
}

bool Lexer::isIdentifierStart( char character ) noexcept
{
	return ( 'a' <= character && character <= 'z' ) || ( 'A' <= character && character <= 'Z' ) || ( '_' == character );
}

bool Lexer::isIdentifierBody( char character ) noexcept
{
	return isIdentifierStart( character ) || isDigit( character );
}

bool Lexer::isDigit( char character ) noexcept
{
	return '0' <= character && character <= '9';
}
