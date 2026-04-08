#pragma once


#include <vector>
#include <cstring>

#include "PacketId.h"
#include "PacketHeader.h"



template< typename T >
std::vector< char >				buildPacket( PacketId id, const T& payload ) noexcept
{
	int32_t totalSize			= static_cast<int32_t>( PACKET_HEADER_SIZE + sizeof( T ) );
	std::vector<char> buffer( totalSize );

    PacketHeader* header        = reinterpret_cast< PacketHeader* >( buffer.data() );
    header->_size               = totalSize;
    header->_packetId           = static_cast< int16_t >( id );

    memcpy( buffer.data() + PACKET_HEADER_SIZE, &payload, sizeof( T ) );

    return buffer;
}