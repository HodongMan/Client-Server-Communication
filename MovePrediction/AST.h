#pragma once


#include "CommonPch.h"


struct FieldNode
{
	std::string								_type;
	std::string								_name;

	bool									_isArray					= false;
	bool									_isString					= false;
};

struct PacketNode
{
	int32_t									_id							= 0;
	std::string								_name;
	
	std::vector< FieldNode >				_fields;

	int32_t									_line						= 0;
};


struct SchemaNode
{
	std::vector< PacketNode >				_packets;
};
