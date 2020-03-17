/* $Id$ */

/**
 * @file http/json.h put a better description here
 */

#ifndef NETWORK_HTTP_JSON_H
#define NETWORK_HTTP_JSON_H

#ifdef ENABLE_NETWORK
#include <list>
#include <sstream>
#include <string>

/** Base handler for all web server requests */
class JSONWriter {
protected:

	std::list<bool> depth_map;
	std::stringstream build_buffer;
	char* built_string = new char[1];

	void Concat(const char* str);
	void ConcatJSON(const char* tpl, ...);
	void ConcatComma();

	void IncreaseDepth();
	void DecreaseDepth();

public:
	JSONWriter();
	JSONWriter(const char* prefix);
	~JSONWriter();

	const char* GetString();

	void AddString(const char* value);
	void AddString(const char* key, const char* value);
	void AddLong(long value);
	void AddLong(const char* key, long value);
	void AddDouble(double value);
	void AddDouble(const char* key, double value);
	void AddBool(bool value);
	void AddBool(const char* key, bool value);
	void AddNull();
	void AddNull(const char* key);

	void StartObject();
	void StartObject(const char* key);

	void StartArray();
	void StartArray(const char* key);

	void EndObject();
	void EndArray();

};

#endif /* ENABLE_NETWORK */

#endif /* NETWORK_HTTP_JSON_H */