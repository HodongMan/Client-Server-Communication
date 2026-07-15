#pragma once


#include <string>
#include <unordered_map>


struct TypeInfo
{
	std::string										_cppType;							// 타입명(Ex : int32_t)
	int32_t											_size						= 0;	// type size

	bool											_isFixedSize				= true;	// fixed인지?

	bool											_isString					= false;
	bool											_isPrimitive				= false;
};

// type 저장소
class TypeRegistry
{
public:
	TypeRegistry( void ) noexcept;

	const TypeInfo*									find( const std::string& idlType ) const noexcept;

	bool											contains( const std::string& idlType ) const noexcept;
	void											registerUserType( const std::string& idlType, const TypeInfo& info ) noexcept;

private:
	void											registerPrimitives( void ) noexcept;

private:
	std::unordered_map< std::string, TypeInfo >		_types;
};