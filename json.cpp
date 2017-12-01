//
// Created by hujianzhe on 16-5-2.
//

#include "json.h"
#include <stdlib.h>

namespace Util {
Json::Json(const Json& o) {}
Json& Json::operator=(const Json& o) { return *this; }
Json::Json(cJSON* root) :
	m_root(root)
{
}

Json::~Json(void) { reset(); }

std::string Json::serialize(bool format) const {
	std::string res;
	do {
		if (!m_root) {
			break;
		}
		char* text = (format ? cJSON_Print(m_root) : cJSON_PrintUnformatted(m_root));
		if (!text) {
			break;
		}
		res = text;

		cJSON_Hooks hk;
		cJSON_GetHooks(&hk);
		hk.free_fn(text);
	} while (0);
	return res;
}

bool Json::deserializeFromFile(const char* path) {
	cJSON* root = cJSON_ParseFromFile(path);
	if (root) {
		reset();
		m_root = root;
		return true;
	}
	return false;
}
bool Json::deserializeFromFile(const std::string& path) {
	return deserializeFromFile(path.c_str());
}
bool Json::deserialize(const char* text) {
	cJSON* root = cJSON_Parse(text);
	if (root) {
		reset();
		m_root = root;
		return true;
	}
	return false;
}
bool Json::deserialize(const std::string& text) {
	return deserialize(text.c_str());
}

cJSON* Json::root(void) const { return m_root; }
cJSON* Json::mutableRoot(void) {
	if (!m_root) {
		m_root = newRoot();
	}
	return m_root;
}
cJSON* Json::newRoot(void) {
	cJSON* root = cJSON_CreateObject();
	if (!root) {
		return NULL;
	}
	reset();
	m_root = root;
	return m_root;
}

void Json::reset(void) {
	cJSON_Delete(m_root);
	m_root = NULL;
}
}
