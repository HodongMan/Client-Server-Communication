#pragma once


#include "TypeRegistry.h"
#include "Ast.h"


class CodeGenerator
{
public:
	CodeGenerator( const SchemaNode& schema, const TypeRegistry& types ) noexcept;

	std::string									generatePacketHeader( void ) noexcept;

	bool										hasError( void ) const noexcept;
	const std::string&							errorMessage( void ) const noexcept;

private:
	void										writeFileHeader( std::ostringstream& out ) const noexcept;
	void										writePacketIdEnum( std::ostringstream& out ) const noexcept;
	void										writePacketStruct( std::ostringstream& out, const PacketNode& packet ) noexcept;

	void										recordError( const std::string& message ) noexcept;

private:
	const SchemaNode&							_schema;
	const TypeRegistry&							_type;

	std::string									_errorMessage;
	bool										_hasError;
};