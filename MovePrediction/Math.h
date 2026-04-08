#pragma once



constexpr float						PI							= 3.14159265359f;
constexpr float						DegToRad					= PI / 180.f;


struct Vector3
{
	float							_x							= 0.0f;
	float							_y							= 0.0f;
	float							_z							= 0.0f;

	Vector3( float x, float y, float z ) noexcept;
	void							add( const Vector3& value ) noexcept;
	void							sub( const Vector3& value ) noexcept;
	void							mul( float scala ) noexcept;

	float							getLength( void ) const noexcept;
	void							normalize( void ) noexcept;
	float							dot ( const Vector3& value ) const noexcept;
	void							cross( const Vector3& value ) noexcept;
};

struct Quaternion
{
	float							_zy							= 0.0f;
	float							_xz							= 0.0f;
	float							_yx							= 0.0f;
	float							_scalar						= 0.0f;

	Quaternion( float zy, float xz, float yx, float scalar ) noexcept;
	void							identity( void ) noexcept;
	void							angleAxis( const Vector3& axis, float angle ) noexcept;
	void							euler( const Vector3& euler ) noexcept;
	Vector3							mul( const Vector3& value ) noexcept;
	void							mul( const Quaternion& rhs ) noexcept;
	Vector3							right( void ) noexcept;
	Vector3							forward( void ) noexcept;
	Vector3							up( void ) noexcept;
};


struct Matrix4x4
{
	float							_m11						= 0.0f;
	float							_m21						= 0.0f;
	float							_m31						= 0.0f;
	float							_m41						= 0.0f;

	float							_m12						= 0.0f;
	float							_m22						= 0.0f;
	float							_m32						= 0.0f;
	float							_m42						= 0.0f;

	float							_m13						= 0.0f;
	float							_m23						= 0.0f;
	float							_m33						= 0.0f;
	float							_m43						= 0.0f;

	float							_m14						= 0.0f;
	float							_m24						= 0.0f;
	float							_m34						= 0.0f;
	float							_m44						= 0.0f;

	void							identity( void ) noexcept;
	void							projection( float fov, float aspectRatio, float nearPlane, float farPlane ) noexcept;
	void							translation( float x, float y, float z ) noexcept;
	void							translation( const Vector3& translationValue ) noexcept;
	void							rotationX( float rotation ) noexcept;
	void							rotationY( float rotation ) noexcept;
	void							rotationZ( float rotation ) noexcept;

	void							rotation( const Quaternion& rotation ) noexcept;
	void							mul( const Matrix4x4& rhs ) noexcept;
	Vector3							mulDirection( const Vector3& rhs ) noexcept;
	void							camera( const Vector3& position, const Vector3& right, const Vector3& forward, const Vector3& up ) noexcept;
	void							lookat( const Vector3& position, const Vector3& target, const Vector3& up ) noexcept;
};