//
// Created by hujianzhe on 22-3-16
//

namespace util {
template <typename T>
struct cstruct_wrap : public T {
	virtual ~cstruct_wrap() {}
};
}
