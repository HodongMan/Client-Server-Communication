#pragma once


#include <string>
#include "CommonPch.h"


enum class TypeToken
{
	PACKET,
	IDENTIFIER,
	NUMBER,
	EQUALS,
	LBRACE,
	RBRACE,
	SEMICOLON,
	END_OF_FILE,
	UNKNOWN,
	MAX,
};

struct Token
{
	TypeToken							_type			= TypeToken::UNKNOWN;
	std::string							_text;
	int32_t								_line			= 0;
	int32_t								_column			= 0;
};

inline const char*						tokenToTypeString( TypeToken type ) noexcept
{
	switch ( type )
	{
	case TypeToken::PACKET:
		{
			return "PACKET";
		}
		break;
	case TypeToken::IDENTIFIER:
		{
			return "IDENTIFIER";
		}
		break;
	case TypeToken::NUMBER:
		{
			return "NUMBER";
		}
		break;
	case TypeToken::EQUALS:
		{
			return "EQUALS";
		}
		break;
	case TypeToken::LBRACE:
		{
			return "LBRACE";
		}
		break;
	case TypeToken::RBRACE:
		{
			return "RBRACE";
		}
		break;
	case TypeToken::SEMICOLON:
		{
			return "SEMICOLON";
		}
		break;
	case TypeToken::END_OF_FILE:
		{
			return "END_OF_FILE";
		}
		break;
	case TypeToken::UNKNOWN:
		{
			return "UNKNOWN";
		}
		break;
	default:
		{
			static_assert( 9 == static_cast< int32_t >( TypeToken::MAX ), "TokenType이 추가되면 여기도 작업 해야 합니다." );
			HDASSERT( false, "Token Type값이 비정상 입니다." );
		}
		break;
	}
}