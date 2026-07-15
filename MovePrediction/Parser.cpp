#include "CommonPch.h"
#include "Parser.h"


Parser::Parser( Lexer& lexer ) noexcept
	: _lexer( lexer )
{

}

SchemaNode Parser::parse( void ) noexcept
{
	SchemaNode schema							= {};

	while ( true )
	{
		Token token								= _lexer.peek();
		if ( TypeToken::END_OF_FILE == token._type )
		{
			break;
		}

		if ( TypeToken::PACKET == token._type )
		{
			PacketNode packet					= parsePacket();
			if ( true == _hasError )
			{
				break;
			}

			schema._packets.emplace_back( std::move( packet ) );
		}
		else
		{
			HDASSERT( false, "최상위에는 'packet' 정의만 올 수 있습니다" );
			recordError( "최상위에는 'packet' 정의만 올 수 있습니다", token );

			break;
		}
	}

	return schema;
}

bool Parser::hasError(void) const noexcept
{
	return false;
}

const std::string& Parser::errorMessage( void ) const noexcept
{
	return _errorMessage;
}

PacketNode Parser::parsePacket( void ) noexcept
{
	PacketNode packet							= {};

	if ( false == expect( TypeToken::PACKET ) )
	{
		return packet;
	}

	Token nameToken								= _lexer.next();
	if ( TypeToken::IDENTIFIER != nameToken._type )
	{
		recordError( "패킷 이름(식별자)을 기대했습니다", nameToken );
		return packet;
	}

	packet._name								= nameToken._text;
	packet._line								= nameToken._line;

	if ( false == expect( TypeToken::EQUALS ) )
	{
		return packet;
	}

	Token idToken								= _lexer.next();
	if ( TypeToken::NUMBER != idToken._type )
	{
		recordError( "패킷 ID(숫자)를 기대했습니다", idToken );
		return packet;
	}

	packet._id									= std::atoi( idToken._text.c_str() );

	// {
	if ( false == expect( TypeToken::LBRACE ) )
	{
		return packet;
	}

	// } 나올때까지
	while ( true )
	{
		Token token								= _lexer.peek();

		if ( TypeToken::RBRACE == token._type )
		{
			break;
		}

		if ( TypeToken::END_OF_FILE == token._type )
		{
			recordError( "'}'가 없이 입력이 끝났습니다.", token );
			return packet;
		}

		FieldNode field							= parseField();

		if ( true == _hasError )
		{
			return packet;
		}

		packet._fields.emplace_back( std::move( field ) );
	}

	// }
	if ( false == expect( TypeToken::RBRACE ) )
	{
		return packet;
	}

	return packet;
}

FieldNode Parser::parseField( void ) noexcept
{
	FieldNode field								= {};

	Token typeToken								= _lexer.next();
	if ( TypeToken::IDENTIFIER != typeToken._type )
	{
		recordError( "필드 타입(식별자)을 기대했습니다", typeToken );
		return field;
	}

	field._type									= typeToken._text;

	// name
	Token nameToken								= _lexer.next();
	if ( TypeToken::IDENTIFIER != nameToken._type )
	{
		recordError( "필드 이름(식별자)을 기대했습니다", nameToken );
		return field;
	}

	field._name									= nameToken._text;

	// ;
	if ( false == expect( TypeToken::SEMICOLON ) )
	{
		return field;
	}

	return field;
}

bool Parser::expect( TypeToken type ) noexcept
{
	HDASSERT( TypeToken::MAX != type, "Type 정보가 비정상 입니다." );

	Token token									= _lexer.next();
	if ( token._type != type )
	{
		HDASSERT( false, "Type이 비정상 입니다." );

		char buffer[ 256 ];
		snprintf( buffer, sizeof( buffer ), "'%s' 토큰을 기대했지만 잘못된 에러가 왔습니다" );
		recordError( buffer, token );
		return false;
	}

	return true;
}

Token Parser::consume( void ) noexcept
{
	return _lexer.next();
}

void Parser::recordError( const std::string& message, const Token& token ) noexcept
{
	HDASSERT( false == message.empty(), "Error Message 정보가 비정상 입니다." );

	if ( true == _hasError )
	{
		return;
	}

	_hasError									= true;

	char buffer[ 512 ];
	snprintf( buffer, sizeof( buffer ), "[%d:%d] %s", token._line, token._column, message.c_str() );

	_errorMessage								= buffer;
}
