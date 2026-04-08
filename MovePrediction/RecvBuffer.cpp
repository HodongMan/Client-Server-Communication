#include "CommonPch.h"
#include "RecvBuffer.h"


RecvBuffer::RecvBuffer( int32_t capacity ) noexcept
	: _capacity{ capacity }
{
	HDASSERT( 0 < capacity, "Capacity 값이 비정상 입니다." );
	_buffer										= new char[ capacity ];
}

RecvBuffer::~RecvBuffer( void ) noexcept
{
	HDASSERT( nullptr != _buffer, "buffer값이 이미 해제되어 있습니다. 비정상 입니다." );
	delete[] _buffer;
}

void RecvBuffer::onRecv( const char* data, int32_t size ) noexcept
{
	HDASSERT( nullptr != data, "Recv Data 값이 비정상 입니다." );
	HDASSERT( 0 < size, "size 값이 비정상 입니다." );
	HDASSERT( size < _capacity, "이럴수가 size값이 너무나도 큽니다." );

	if ( _capacity < _writePosition + size )
	{
		compact();
		if ( _capacity < _writePosition + size )
		{
			HDASSERT( false, "버퍼가 넘칩니다. 비정상 입니다." );
			return;
		}
	}

	::memcpy( &_buffer[ _writePosition ], data, size );
	_writePosition								+= size;
}

// 검증 없이 copy도 안하고 writePosition만 증가시키는 함수입니다

void RecvBuffer::onRecv( int32_t size ) noexcept
{
	HDASSERT( 0 < size, "size 값이 비정상 입니다." );
	HDASSERT( size < _capacity, "이럴수가 size값이 너무나도 큽니다." );

	_writePosition								+= size;
}

bool RecvBuffer::tryGetPacket( Packet& outPacket ) noexcept
{
	//HDASSERT( 0 == outPacket._id, "PacketId가 초기화 된 상태로 와야 합니다" );
	//HDASSERT( true == outPacket._data.empty(), "나중에 재사용을 허용한다면 제거 예정. 지금은 문제 입니다." );

	const int32_t dataSize						= _writePosition - _readPosition;
	if ( dataSize < PACKET_HEADER_SIZE )
	{
		//HDASSERT( false, "PACKET Header도 얻을 수 없습니다. 패킷을 넣어놓지 않은거 같은데요" );
		return false;
	}

	const PacketHeader* header					= reinterpret_cast< const PacketHeader* >( &_buffer[ _readPosition ] );
	HDASSERT( nullptr != header, "이 데이터가 비정상 인것은 뭔가 데이터가 버퍼런 한거 같습니다." );
	if ( dataSize < header->_size )
	{
		HDASSERT( false, "PACEKT DATA가 전부 오지 않았습니다." );
		return false;
	}

	// copy packet
	outPacket._id								= header->_packetId;
	const int32_t bodySize						= header->_size - PACKET_HEADER_SIZE;
	HDASSERT( 0 < bodySize, "본문의 내용 없이 header만 오는 경우는 생각하진 않았습니다. 나중에 있으면 제거할 것!" );

	if ( 0 < bodySize )
	{
		outPacket._data.resize( bodySize );
		::memcpy( outPacket._data.data(), &_buffer[ _readPosition + PACKET_HEADER_SIZE ], bodySize );
	}

	_readPosition								+= header->_size;
	
	return true;
}

bool RecvBuffer::tryGetPacket( PacketView& outPacket ) noexcept
{
	//HDASSERT( 0 == outPacket._id, "PacketId가 초기화 된 상태로 와야 합니다" );
	//HDASSERT( true == outPacket._data.empty(), "나중에 재사용을 허용한다면 제거 예정. 지금은 문제 입니다." );

	const int32_t dataSize						= _writePosition - _readPosition;
	if ( dataSize < PACKET_HEADER_SIZE )
	{
		//HDASSERT( false, "PACKET Header도 얻을 수 없습니다. 패킷을 넣어놓지 않은거 같은데요" );
		return false;
	}

	const PacketHeader* header					= reinterpret_cast< const PacketHeader* >( &_buffer[ _readPosition ] );
	HDASSERT( nullptr != header, "이 데이터가 비정상 인것은 뭔가 데이터가 버퍼런 한거 같습니다." );
	if ( dataSize < header->_size )
	{
		HDASSERT( false, "PACEKT DATA가 전부 오지 않았습니다." );
		return false;
	}

	// copy packet
	outPacket._id								= header->_packetId;
	outPacket._size								= header->_size;
	
	// 복사가 아니라 참조를 가져오는 버전
	outPacket._data								= &_buffer[ _readPosition ];

	_readPosition								+= header->_size;
	
	return true;
}

void RecvBuffer::compact( void ) noexcept
{
	HDASSERT( 0 != _readPosition, "일단 읽어야 할 것이 없는데 compact 함수를 호출했으면 로직을 다시 돌아봐야 합니다." );
	if ( 0 == _readPosition )
	{
		return;
	}

	const int32_t remain						= _writePosition - _readPosition;
	if ( 0 < remain )
	{
		memmove( _buffer, &_buffer[ _readPosition ], remain );
	}

	_readPosition								= 0;
	_writePosition								= remain;
}

char* RecvBuffer::getWritePtr( void ) noexcept
{
	HDASSERT( _writePosition <= _capacity, "Write Position이 비정상입니다. 크래시 발생할 예정" );

	return &_buffer[ _writePosition ];
}

int32_t RecvBuffer::getFreeSize( void ) const noexcept
{
	return _capacity - _writePosition;
}
