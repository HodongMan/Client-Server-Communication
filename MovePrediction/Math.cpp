#include "CommonPch.h"
#include "Math.h"

Vector3::Vector3( float x, float y, float z ) noexcept
	: _x{ x }
	, _y{ y }
	, _z{ z }
{

}

void Vector3::add( const Vector3& value ) noexcept
{
	_x									+= value._x;
	_y									+= value._y;
	_z									+= value._z;
}

void Vector3::sub( const Vector3& value ) noexcept
{
	_x									-= value._x;
	_y									-= value._y;
	_z									-= value._z;
}

void Vector3::mul( float scala ) noexcept
{
	_x									*= scala;
	_y									*= scala;
	_z									*= scala;
}

float Vector3::getLength( void ) const noexcept
{
	return ( _x * _x ) + ( _y * _y ) + ( _z * _z );
}

void Vector3::normalize( void ) noexcept
{
	const float lengthSquare			= getLength();
	if ( 0.0f < lengthSquare )
	{
		float invLength					= 1 / ( float )sqrt( lengthSquare );
		mul( invLength );
	}
}

float Vector3::dot( const Vector3& value ) const noexcept
{
	return ( _x * value._x ) + ( _y * value._y ) + ( _z * value._z );
}

void Vector3::cross( const Vector3& value ) noexcept
{
	float x								= ( _y * value._z ) - ( _z * value._y );
	float y								= ( _z * value._x ) - ( _x * value._z );
	float z								= ( _x * value._y ) - ( _y * value._x );

	_x									= x;
	_y									= y;
	_z									= z;
}

void Matrix4x4::identity( void ) noexcept
{
	_m11								= 1.0f;
	_m21								= 0.0f;
	_m31								= 0.0f;
	_m41								= 0.0f;

	_m12								= 0.0f;
	_m22								= 1.0f;
	_m32								= 0.0f;
	_m42								= 0.0f;

	_m13								= 0.0f;
	_m23								= 0.0f;
	_m33								= 1.0f;
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= 0.0f;
	_m44								= 1.0f;
}

void Matrix4x4::projection( float fov, float aspectRatio, float nearPlane, float farPlane ) noexcept
{
	_m11								= 1.0f / ( tanf( fov * 0.5f ) * aspectRatio );
	_m21								= 0.0f;
	_m31								= 0.0f;
	_m41								= 0.0f;

	_m12								= 0.0f;
	_m22								= 0.0f;
	_m32								= ( farPlane / ( farPlane - nearPlane ) );
	_m42								= 1.0f;

	_m13								= 0.0f;
	_m23								= -1.0f / tanf( fov * 0.5f );
	_m33								= 0.0f;
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= ( nearPlane * farPlane ) / ( nearPlane - farPlane );
	_m44								= 0.0f;
}

void Matrix4x4::translation( float x, float y, float z ) noexcept
{
	_m11								= 1.0f;
	_m21								= 0.0f;
	_m31								= 0.0f;
	_m41								= 0.0f;

	_m12								= 0.0f;
	_m22								= 1.0f;
	_m32								= 0.0f;
	_m42								= 0.0f;

	_m13								= 0.0f;
	_m23								= 0.0f;
	_m33								= 1.0f;
	_m43								= 0.0f;

	_m14								= x;
	_m24								= y;
	_m34								= z;
	_m44								= 1.0f;
}

void Matrix4x4::translation( const Vector3& translationValue ) noexcept
{
	translation( translationValue._x, translationValue._y, translationValue._z );
}

void Matrix4x4::rotationX( float rotation ) noexcept
{
	float cosRotation					= cosf( rotation );
	float sinRotation					= sinf( rotation );

	_m11								= 1.0f;
	_m21								= 0.0f;
	_m31								= 0.0f;
	_m41								= 0.0f;

	_m12								= 0.0f;
	_m22								= cosRotation;
	_m32								= sinRotation;
	_m42								= 0.0f;

	_m13								= 0.0f;
	_m23								= -sinRotation;
	_m33								= cosRotation;
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= 0.0f;
	_m44								= 1.0f;
}

void Matrix4x4::rotationY( float rotation ) noexcept
{
	float cosRotation					= cosf( rotation );
	float sinRotation					= sinf( rotation );

	_m11								= cosRotation;
	_m21								= 0.0f;
	_m31								= -sinRotation;
	_m41								= 0.0f;

	_m12								= 0.0f;
	_m22								= 1.0f;
	_m32								= 0.0f;
	_m42								= 0.0f;

	_m13								= sinRotation;
	_m23								= 0.0f;
	_m33								= cosRotation;
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= 0.0f;
	_m44								= 1.0f;
}

void Matrix4x4::rotationZ( float rotation ) noexcept
{
	float cosRotation					= cosf( rotation );
	float sinRotation					= sinf( rotation );

	_m11								= cosRotation;
	_m21								= sinRotation;
	_m31								= 0.0f;
	_m41								= 0.0f;

	_m12								= -sinRotation;
	_m22								= cosRotation;
	_m32								= 0.0f;
	_m42								= 0.0f;

	_m13								= 0.0f;
	_m23								= 0.0f;
	_m33								= 1.0f;
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= 0.0f;
	_m44								= 1.0f;
}

void Matrix4x4::rotation( const Quaternion& rotation ) noexcept
{
	_m11								= ( rotation._scalar * rotation._scalar ) + ( rotation._zy * rotation._zy ) + ( -1.0f * rotation._xz * rotation._xz ) + ( -1.0f * rotation._yx * rotation._yx );
	_m21								= ( 2.0f * rotation._scalar * rotation._yx ) + ( 2.0f * rotation._zy * rotation._xz );
	_m31								= ( -2.0f * rotation._scalar * rotation._xz ) + ( 2.0f * rotation._zy * rotation._yx );
	_m41								= 0.0f;

	_m12								= ( -2.0f * rotation._scalar * rotation._yx ) + ( 2.0f * rotation._zy * rotation._xz );
	_m22								= ( rotation._scalar * rotation._scalar ) + ( -1.0f * rotation._zy * rotation._zy ) + ( rotation._xz * rotation._xz ) + ( -1.0f * rotation._yx * rotation._yx );
	_m32								= ( 2.0f * rotation._scalar * rotation._zy ) + ( 2.0f * rotation._xz * rotation._yx );
	_m42								= 0.0f;

	_m13								= ( 2.0f * rotation._scalar * rotation._xz ) + ( 2.0f * rotation._zy * rotation._yx );
	_m23								= ( -2.0f * rotation._scalar * rotation._zy ) + ( 2.0f * rotation._xz * rotation._yx );
	_m33								= ( rotation._scalar * rotation._scalar ) + ( -1.0f * rotation._zy * rotation._zy ) + ( -1.0f * rotation._xz * rotation._xz ) + ( rotation._yx * rotation._yx );
	_m43								= 0.0f;

	_m14								= 0.0f;
	_m24								= 0.0f;
	_m34								= 0.0f;
	_m44								= 1.0f;
}

void Matrix4x4::mul( const Matrix4x4& rhs ) noexcept
{
	float m11							= ( _m11 * rhs._m11 ) + ( _m12 * rhs._m21 ) + ( _m13 * rhs._m31 ) + ( _m14 * rhs._m41 );
	float m21							= ( _m21 * rhs._m11 ) + ( _m22 * rhs._m21 ) + ( _m23 * rhs._m31 ) + ( _m24 * rhs._m41 );
	float m31							= ( _m31 * rhs._m11 ) + ( _m32 * rhs._m21 ) + ( _m33 * rhs._m31 ) + ( _m34 * rhs._m41 );
	float m41							= ( _m41 * rhs._m11 ) + ( _m42 * rhs._m21 ) + ( _m43 * rhs._m31 ) + ( _m44 * rhs._m41 );

	float m12							= ( _m11 * rhs._m12 ) + ( _m12 * rhs._m22 ) + ( _m13 * rhs._m32 ) + ( _m14 * rhs._m42 );
	float m22							= ( _m21 * rhs._m12 ) + ( _m22 * rhs._m22 ) + ( _m23 * rhs._m32 ) + ( _m24 * rhs._m42 );
	float m32							= ( _m31 * rhs._m12 ) + ( _m32 * rhs._m22 ) + ( _m33 * rhs._m32 ) + ( _m34 * rhs._m42 );
	float m42							= ( _m41 * rhs._m12 ) + ( _m42 * rhs._m22 ) + ( _m43 * rhs._m32 ) + ( _m44 * rhs._m42 );

	float m13							= ( _m11 * rhs._m13 ) + ( _m12 * rhs._m23 ) + ( _m13 * rhs._m33 ) + ( _m14 * rhs._m43 );
	float m23							= ( _m21 * rhs._m13 ) + ( _m22 * rhs._m23 ) + ( _m23 * rhs._m33 ) + ( _m24 * rhs._m43 );
	float m33							= ( _m31 * rhs._m13 ) + ( _m32 * rhs._m23 ) + ( _m33 * rhs._m33 ) + ( _m34 * rhs._m43 );
	float m43							= ( _m41 * rhs._m13 ) + ( _m42 * rhs._m23 ) + ( _m43 * rhs._m33 ) + ( _m44 * rhs._m43 );

	float m14							= ( _m11 * rhs._m14 ) + ( _m12 * rhs._m24 ) + ( _m13 * rhs._m34 ) + ( _m14 * rhs._m44 );
	float m24							= ( _m21 * rhs._m14 ) + ( _m22 * rhs._m24 ) + ( _m23 * rhs._m34 ) + ( _m24 * rhs._m44 );
	float m34							= ( _m31 * rhs._m14 ) + ( _m32 * rhs._m24 ) + ( _m33 * rhs._m34 ) + ( _m34 * rhs._m44 );
	float m44							= ( _m41 * rhs._m14 ) + ( _m42 * rhs._m24 ) + ( _m43 * rhs._m34 ) + ( _m44 * rhs._m44 );

	_m11								= m11;
	_m21								= m21;
	_m31								= m31;
	_m41								= m41;

	_m12								= m12;
	_m22								= m22;
	_m32								= m32;
	_m42								= m42;

	_m13								= m13;
	_m23								= m23;
	_m33								= m33;
	_m43								= m43;

	_m14								= m14;
	_m24								= m24;
	_m34								= m34;
	_m44								= m44;
}

Vector3 Matrix4x4::mulDirection( const Vector3& rhs ) noexcept
{
	return Vector3( ( rhs._x * _m11 ) + ( rhs._y * _m12 ) + ( rhs._z * _m13 ), ( rhs._x * _m21 ) + ( rhs._y * _m22 ) + ( rhs._z * _m23 ), ( rhs._x * _m31 ) + ( rhs._y * _m32 ) + ( rhs._z * _m33 ));
}

void Matrix4x4::camera( const Vector3& position, const Vector3& right, const Vector3& forward, const Vector3& up ) noexcept
{
	Vector3 translation					= position;
	translation.mul( -1.0f );

	_m11								= right._x;
	_m21								= forward._x;
	_m31								= up._x;
	_m41								= 0.0f;

	_m12								= right._y;
	_m22								= forward._y;
	_m32								= up._y;
	_m42								= 0.0f;

	_m13								= right._z;
	_m23								= forward._z;
	_m33								= up._z;
	_m43								= 0.0f;

	_m14								= ( right._x * translation._x ) + ( right._y * translation._y ) + ( right._z * translation._z );
	_m24								= ( forward._x * translation._x ) + ( forward._y * translation._y ) + ( forward._z * translation._z );
	_m34								= ( up._x * translation._x ) + ( up._y * translation._y ) + ( up._z * translation._z );
	_m44								= 1.0f;
}

void Matrix4x4::lookat( const Vector3& position, const Vector3& target, const Vector3& up ) noexcept
{
	Vector3 viewForward					= target;
	viewForward.sub( position );
	viewForward.normalize();

	Vector3 projectUpOntoForward		= viewForward;
	projectUpOntoForward.mul( up.dot( viewForward ) );

	Vector3 viewUp						= up;
	viewUp.sub( projectUpOntoForward );
	viewUp.normalize();

	Vector3 viewRight					= viewForward;
	viewRight.cross( viewUp );

	camera( position, viewRight, viewForward, viewUp );
}

Quaternion::Quaternion( float zy, float xz, float yx, float scalar ) noexcept
	: _zy{ zy }
	, _xz{ xz }
	, _yx{ yx }
	, _scalar{ scalar }
{

}

void Quaternion::identity( void ) noexcept
{
	_zy									= 0.0f;
	_xz									= 0.0f;
	_yx									= 0.0f;
	_scalar								= 1.0f;
}

void Quaternion::angleAxis( const Vector3& axis, float angle ) noexcept
{
	float halfTheta						= angle * 0.5f;
	float sinHalfTheta					= sinf( halfTheta );

	_zy									= axis._x * sinHalfTheta;
	_xz									= axis._y * sinHalfTheta;
	_yx									= axis._z * sinHalfTheta;
	_scalar								= cosf( halfTheta );
}

void Quaternion::euler( const Vector3& euler ) noexcept
{
	Quaternion pitch					= Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	pitch.angleAxis( Vector3( 1.0f, 0.0f, 0.0f ), euler._x );

	Quaternion yaw						= Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	yaw.angleAxis( Vector3( 0.0f, 1.0f, 0.0f ), euler._y );

	Quaternion roll						= Quaternion( 0.0f, 0.0f, 0.0f, 0.0f );
	yaw.angleAxis( Vector3( 0.0f, 0.0f, 1.0f ), euler._z );
	
	pitch.mul( yaw );

	return yaw.mul( pitch );

}

Vector3 Quaternion::mul( const Vector3& value ) noexcept
{
	float x								= ( _scalar * _scalar * value._x ) + ( -2.0f * _scalar * _yx * value._y ) + ( 2.0f * _scalar * _xz * value._z ) + ( -_zy * -_zy * value._x ) + ( 2.0f * -_zy * -_xz * value._y ) + ( 2.0f * -_zy * _yx * value._z ) + ( -1.0f * -_xz * -_xz * value._x ) + ( -1.0f * _yx * _yx * value._x );
	float y								= ( 2.0f * _scalar * _yx * value._x ) + ( _scalar * _scalar * value._y ) + ( -2.0f * _scalar * -_zy * value._z ) + ( 2.0f * -_zy * -_xz * value._x ) + ( -1.0f * -_zy * -_zy * value._y ) + ( -_xz * -_xz * value._y ) + ( 2.0f * -_xz * _yx * value._z ) + ( -1.0f * _yx * _yx * value._y );
	float z								= ( -2.0f * _scalar * -_xz * value._x ) + ( 2.0f * _scalar * -_zy * value._y ) + ( _scalar * _scalar * value._z ) + ( 2.0f * -_zy * _yx * value._x ) + ( -1.0f * -_zy * -_zy * value._z ) + ( 2.0f * -_xz * _yx * value._y ) + ( -1.0f * -_xz * -_xz * value._z ) + ( _yx * _yx * value._z );

	return Vector3( x, y, z );
}

void Quaternion::mul( const Quaternion& rhs ) noexcept
{
	float zy							= ( rhs._zy * _scalar ) + ( rhs._scalar * _zy ) + ( rhs._yx * _xz ) + ( -1.0f * _yx * rhs._xz );
	float xz							= ( rhs._xz * _scalar ) + ( -1.0f * rhs._yx * _zy ) + ( rhs._scalar * _xz ) + ( _yx * rhs._zy );
	float yx							= ( rhs._yx * _scalar ) + ( rhs._xz * _zy ) + ( -1.0f * rhs._zy * _xz ) + ( _yx * rhs._scalar );
	float scalar						= ( rhs._scalar * _scalar ) + ( -1.0f * rhs._zy * _zy ) + ( -1.0f * rhs._xz * _xz ) + ( -1.0f * _yx * rhs._yx );

	_zy									= zy;
	_xz									= xz;
	_yx									= yx;
	_scalar								= scalar;
}

Vector3 Quaternion::right( void ) noexcept
{
	return Vector3( ( _scalar * _scalar ) + ( -_zy * -_zy ) + ( -1.0f * -_xz * -_xz ) + ( -1.0f * _yx * _yx ), ( 2.0f * _scalar * _yx ) + ( 2.0f * -_zy * -_xz ), ( -2.0f * _scalar * -_xz ) + ( -2.0f * _scalar * -_xz ) + ( 2.0f * -_zy * _yx ) );
}

Vector3 Quaternion::forward( void ) noexcept
{
	return Vector3( ( -2.0f * _scalar * _yx ) + ( 2.0f * -_zy * -_xz ), ( _scalar * _scalar ) + ( -1.0f * -_zy * _zy ) + ( -_xz * -_xz ) + ( -1.0f * _yx * _yx ), ( 2.0f * _scalar * -_zy ) + ( 2.0f * -_xz * _yx  ) );
}

Vector3 Quaternion::up( void ) noexcept
{
	return Vector3( ( 2.0f * _scalar * -_xz ) + ( 2.0f * -_zy * _yx ), ( -2.0f * _scalar * -_zy ) + ( 2.0f * -_xz * _yx ), ( _scalar * _scalar ) + ( -1.0f * -_zy * -_zy ) + ( -1.0f * -_xz * -_xz ) + ( _yx * _yx ) );
}
