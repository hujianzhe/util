//
// Created by hujianzhe on 16-12-24.
//

#ifndef UTIL_CPP_JSON_H
#define UTIL_CPP_JSON_H

#include "../c/component/cJSON.h"
#include <string>

namespace Util {
class Json {
public:
	Json(cJSON* root = NULL);
	~Json(void);

	std::string serialize(bool format = false) const;

	bool deserializeFromFile(const char* path);
	bool deserializeFromFile(const std::string& path);
	bool deserialize(const char* text);
	bool deserialize(const std::string& text);

	cJSON* root(void) const;
	cJSON* mutableRoot(void);

private:
	Json(const Json& o);
	Json& operator=(const Json& o);

	cJSON* newRoot(void);
	void reset(void);

private:
	cJSON* m_root;
};
}

#endif
