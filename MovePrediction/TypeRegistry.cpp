#include "CommonPch.h"
#include "TypeRegistry.h"


TypeRegistry::TypeRegistry( void ) noexcept
{
	registerPrimitives();
}

const TypeInfo* TypeRegistry::find( const std::string& idlType ) const noexcept
{
	auto iter									= _types.find( idlType );
	if ( iter == _types.end() )
	{
		return nullptr;
	}

	return &iter->second;
}

bool TypeRegistry::contains( const std::string& idlType ) const noexcept
{
	return _types.find( idlType ) != _types.end();
}

void TypeRegistry::registerUserType( const std::string& idlType, const TypeInfo& info ) noexcept
{
	_types[ idlType ]							= info;
}

void TypeRegistry::registerPrimitives( void ) noexcept
{
	// 정수
	_types[ "int8" ]							= { "int8_t", 1, true, false, true };
	_types[ "int16" ]							= { "int16_t", 2, true, false, true };
	_types[ "int32" ]							= { "int32_t", 4, true, false, true };
	_types[ "int64" ]							= { "int64_t", 8, true, false, true };

	_types[ "uint8" ]							= { "uint8_t", 1, true, false, true };
	_types[ "uint16" ]							= { "uint16_t", 2, true, false, true };
	_types[ "uint32" ]							= { "uint32_t", 4, true, false, true };
	_types[ "uint64" ]							= { "uint64_t", 8, true, false, true };

	// 부동 소수점
	_types[ "float" ]							= { "float", 4, true, false, true };
	_types[ "double" ]							= { "double", 8, true, false, true };

	// 
	_types[ "bool" ]							= { "bool", 1, true, false, true };

	// 가변 문자열
	_types[ "string" ]							= { "std::string", -1, false, true, true };
}
