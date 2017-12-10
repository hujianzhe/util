//
// Created by hujianzhe on 17-4-9.
//

#include "bytebuf.h"
#include <string.h>

namespace Util {
ByteBuf::ByteBuf(void) :
	data(NULL),
	datalen(0),
	m_memlen(0),
	m_isRef(true)
{
}

ByteBuf::~ByteBuf(void) {
	freeData();
}

void ByteBuf::freeData(void) {
	if (!m_isRef) {
		m_isRef = true;
		m_memlen = 0;
		delete[] data;
	}
	data = NULL;
	datalen = 0;
}

void ByteBuf::copy(const void* data, size_t len, bool is_string) {
	if (!data || 0 == len) {
		freeData();
		return;
	}
	size_t real_len = len;
	if (is_string && 0 == ((char*)data)[len - 1]) {
		++real_len;
	}
	if (real_len > m_memlen) {
		freeData();
		this->data = new unsigned char[real_len];
		m_isRef = false;
		m_memlen = real_len;
	}
	this->datalen = real_len;
	memcpy(this->data, data, len);
	if (real_len > len) {
		((char*)(this->data))[len] = 0;
	}
}
void ByteBuf::copy(const std::string& str) { copy(str.data(), str.size(), true); }
void ByteBuf::copy(const std::vector<unsigned char>& data) {
	if (!data.empty()) {
		copy(&data[0], data.size(), false);
	}
	else {
		copy(NULL, 0, false);
	}
}
void ByteBuf::copy(const std::vector<char>& data) {
	if (!data.empty()) {
		copy(&data[0], data.size(), false);
	}
	else {
		copy(NULL, 0, false);
	}
}

void ByteBuf::ref(const void* data, size_t len) {
	freeData();
	if (!data || 0 == len) {
		return;
	}
	this->data = (unsigned char*)data;
	this->datalen = len;
}
void ByteBuf::ref(const std::string& str) { ref(str.data(), str.size()); }
void ByteBuf::ref(const std::vector<unsigned char>& data) {
	if (!data.empty()) {
		ref(&data[0], data.size());
	}
	else {
		ref(NULL, 0);
	}
}
void ByteBuf::ref(const std::vector<char>& data) {
	if (!data.empty()) {
		ref(&data[0], data.size());
	}
	else {
		ref(NULL, 0);
	}
}
}
