//
// Created by hujianzhe on 22-3-16
//

namespace util {
// if T don't have virtual table, T* don't cast to cstruct_wrap<T>*
template <typename T>
struct cstruct_wrap : public T {
	virtual ~cstruct_wrap() {}
};
}
