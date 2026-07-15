#pragma once


#include "Token.h"


class Lexer
{
public:
	explicit Lexer( std::string source ) noexcept;

	Token											next( void ) noexcept;
	Token											peek( void ) noexcept;

private:
	void											skipWhitespaceAndComments( void ) noexcept;
	
	Token											readIdentifierOrKeyword( void ) noexcept;
	Token											readNumber( void ) noexcept;
	Token											makeToken( TypeToken type, std::string text, int32_t line, int32_t column ) const noexcept;
	
	char											current( void ) const noexcept;
	char											lookahead( void ) const noexcept;

	bool											isEnd( void ) const noexcept;
	void											advance( void ) noexcept;

	static bool										isIdentifierStart( char character ) noexcept;
	static bool										isIdentifierBody( char character ) noexcept;
	static bool										isDigit( char character ) noexcept;

private:
	std::string										_source;
	int64_t											_position				= 0;
	int32_t											_line					= 0;
	int32_t											_column					= 0;

	Token											_peeked					= {};
	bool											_hasPeeked				= false;
};