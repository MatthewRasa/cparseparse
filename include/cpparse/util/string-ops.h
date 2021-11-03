/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_UTIL_STRING_OPS_H_
#define CPPARSE_UTIL_STRING_OPS_H_

#include <algorithm>
#include <stdexcept>
#include <string>

namespace cpparse {

	/**
 	 * std::stoull overload to prevent unsigned integer wrap-around.
	 */
	inline unsigned long long stoull(const std::string &value) {
		if (value.find('-') != std::string::npos)
			throw std::out_of_range{""};
		return std::stoull(value);
	}

	/**
	 * Convert string to all uppercase characters.
	 */
	inline std::string str_to_upper(const std::string &str) {
		std::string rtn;
		std::transform(str.begin(), str.end(), std::back_inserter(rtn), toupper);
		return rtn;
	}

}

#endif /* CPPARSE_UTIL_STRING_OPS_H_ */
