/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_UTIL_COMPAT_H_
#define CPPARSE_UTIL_COMPAT_H_

#include <memory>

namespace cpparse {

#if __cplusplus >= 201402L
	using std::make_unique;
#elif __cplusplus >= 201103L
	template<class T, class ...Args>
	std::unique_ptr<T> make_unique(Args&&... args) {
		return std::unique_ptr<T>(new T{std::forward<Args>(args)...});
	}
#else
#error "Unsupported C++ standard; minimum supported standard is C++11"
#endif

}

#endif /* CPPARSE_UTIL_COMPAT_H_ */
