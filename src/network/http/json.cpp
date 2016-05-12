/* $Id$ */

/**
 * @file http/json.h put a better description here
 */

#ifdef ENABLE_NETWORK

#include <stdarg.h>

#include "../../stdafx.h"
#include "../../debug.h"
#include "../../string_func.h"
#include "../../../3rdparty/mongoose/mongoose.h"
#include "json.h"
#include "../../safeguards.h"

JSONWriter::JSONWriter() {
	this->depth_map.push_back(false);
}
JSONWriter::JSONWriter(const char* prefix)
{
	this->depth_map.push_back(false);
	this->Concat(prefix);
}

void JSONWriter::Concat(const char* str)
{
	buffer << str;
	// int buffer_size = strlen(this->buffer) + strlen(str) + 1;
	// char *concat_stream = new char[buffer_size];
	//
	//
	// strecpy( concat_stream, buffer, &concat_stream[buffer_size - 1] );
	// strecat( concat_stream, str, &concat_stream[buffer_size - 1] );
	//
	// if (this->buffer != NULL) delete[] this->buffer;
	// this->buffer = concat_stream;
}

void JSONWriter::ConcatJSON(const char* tpl, ...)
{
	va_list args;
    va_start(args, tpl);
	char temp[1024 + 32];
	json_emit_va(temp, sizeof(temp), tpl, args);
	this->ConcatComma();
	this->Concat(temp);
    va_end(args);
}

const char* JSONWriter::GetString()
{
	return this->buffer.str().c_str();
}

void JSONWriter::AddString(const char* value)
{
	this->ConcatJSON("s", value);
}

void JSONWriter::AddString(const char* key, const char* value)
{
	this->ConcatJSON("s: s", key, value);
}

void JSONWriter::AddLong(long value)
{
	this->ConcatJSON("i", value);
}

void JSONWriter::AddLong(const char* key, long value)
{
	this->ConcatJSON("s: i", key, value);
}

void JSONWriter::AddDouble(double value)
{
	this->ConcatJSON("f", value);
}

void JSONWriter::AddDouble(const char* key, double value)
{
	this->ConcatJSON("s: f", key, value);
}

void JSONWriter::AddBool(bool value)
{
	this->ConcatJSON((value == true) ? "T" : "F");
}

void JSONWriter::AddBool(const char* key, bool value)
{
	this->ConcatJSON((value == true) ? "s: T" : "s: F", key);
}

void JSONWriter::AddNull()
{
	this->ConcatJSON("N");
}

void JSONWriter::AddNull(const char* key)
{
	this->ConcatJSON("s: N", key);
}

void JSONWriter::ConcatComma()
{
	// If not the first one,
	if(this->depth_map.back() == true)
		this->Concat(", ");

	this->depth_map.back() = true;
}

void JSONWriter::IncreaseDepth()
{
	// this->depth++;

	// bool* temp_depth_map = new bool[this->depth];
	// memcpy(&temp_depth_map, &depth_map, this->depth * sizeof(bool));
	//
	// if (this->depth_map != NULL) delete[] this->depth_map;
	// this->depth_map = temp_depth_map;
	// this->depth_map[this->depth] = false;
	this->depth_map.push_back(false);
}

void JSONWriter::DecreaseDepth()
{
	this->depth_map.pop_back();
	// this->depth--;
}

void JSONWriter::StartObject()
{
	this->ConcatComma();
	this->IncreaseDepth();
	this->Concat("{");
}

void JSONWriter::StartObject(const char* key)
{
	this->ConcatJSON("s: {", key);
	this->IncreaseDepth();
}

void JSONWriter::StartArray()
{
	this->ConcatComma();
	this->Concat("[");
	this->IncreaseDepth();
}

void JSONWriter::StartArray(const char* key)
{
	this->ConcatJSON("s: [", key);
	this->IncreaseDepth();
}

void JSONWriter::EndObject()
{
	this->Concat("}");
	this->DecreaseDepth();
}

void JSONWriter::EndArray()
{
	this->Concat("]");
	this->DecreaseDepth();
}

#endif /* ENABLE_NETWORK */
