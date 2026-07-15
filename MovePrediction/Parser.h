#pragma once


#include "Lexer.h"
#include "AST.h"


class Parser
{
public:
	explicit Parser( Lexer& lexer ) noexcept;

	SchemaNode							parse( void ) noexcept;

	bool								hasError( void ) const noexcept;
	const std::string&					errorMessage( void ) const noexcept;

private:
	PacketNode							parsePacket( void ) noexcept;
	FieldNode							parseField( void ) noexcept;

	bool								expect( TypeToken type ) noexcept;
	
	Token								consume( void ) noexcept;

	void								recordError( const std::string& message, const Token& token ) noexcept;

private:
	Lexer&								_lexer;
	
	std::string							_errorMessage;
	bool								_hasError						= false;
};