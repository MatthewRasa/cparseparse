/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPARSEPARSE_UTIL_ERRSTR_H_
#define CPARSEPARSE_UTIL_ERRSTR_H_

#include <sstream>
#include <string>

namespace cpparse {

	std::string _scriptname;

	/**
	 * Set script name for error print-outs.
	 */
	template<class String_Ref>
	void errstr_set_script_name(String_Ref &&name) {
		_scriptname = std::forward<String_Ref>(name);
	}

	/**
	 * Recursive helpers for errstr()/lerrstr().
	 */
	template<class Arg>
	void _errstr(std::stringstream &ss, Arg &&arg) {
		ss << std::forward<Arg>(arg);
	}
	template<class Arg, class ...Args>
	void _errstr(std::stringstream &ss, Arg &&arg, Args&&... args) {
		_errstr(ss, std::forward<Arg>(arg));
		_errstr(ss, std::forward<Args>(args)...);
	}

	/**
	 * Concatenate words into a logic error string.
	 */
	template<class ...Args>
	std::string lerrstr(Args&&... args) {
		std::stringstream ss;
		_errstr(ss, "Argument_Parser: ", std::forward<Args>(args)...);
		return ss.str();
	}

	/**
	 * Concatenate words into a runtime error string.
	 */
	template<class ...Args>
	std::string errstr(Args&&... args) {
		std::stringstream ss;
		_errstr(ss, _scriptname, ": ", std::forward<Args>(args)...);
		return ss.str();
	}

}

#endif /* CPARSEPARSE_UTIL_ERRSTR_H_ */
