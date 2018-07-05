//
// Created by hujianzhe on 16-5-2.
//

#include "json.h"

namespace Util {
char* Json::serialize(bool format) {
	if (!m_root) {
		return "";
	}
	char* s = format ? cJSON_Print(m_root) : cJSON_PrintUnformatted(m_root);
	if (!s) {
		return "";
	}
	cJSON_FreeString(m_str);
	m_str = s;
	return m_str;
}
bool Json::deserialize(const char* text) {
	cJSON* root = cJSON_Parse(NULL, text);
	if (root) {
		cJSON_Delete(m_root);
		m_root = root;
		return true;
	}
	return false;
}
bool Json::deserializeFromFile(const char* path) {
	cJSON* root = cJSON_ParseFromFile(NULL, path);
	if (root) {
		cJSON_Delete(m_root);
		m_root = root;
		return true;
	}
	return false;
}
}
