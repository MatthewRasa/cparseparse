/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_OPTIONAL_INFO_H_
#define CPPARSE_OPTIONAL_INFO_H_

#include "cpparse/argument-info.h"
#include <algorithm>
#include <vector>

namespace cpparse {

	/**
	 * Optional info object storing information about an optional argument.
	 *
	 * Provides functions for setting additional argument properties.
	 */
	class Optional_Info : public Argument_Info<Optional_Info> {
	public:

		/**
		 * Optional argument type.
		 *
		 * FLAG    flag argument; arg() returns true when present or false
		 *         otherwise.
		 *
		 * SINGLE  optional argument with a single value.
		 *
		 * APPEND  optional argument that can be specified multiple times, each time
		 *         appending values to an argument list. Argument values can be
		 *         retrieved with arg_at().
		 */
		enum class Type { FLAG, SINGLE, APPEND };

		template<class String>
		explicit Optional_Info(String &&name, Type type) noexcept
				: Argument_Info{std::forward<String>(name)},
				  m_flag{0},
				  m_type{type} { }

		operator bool() const noexcept {
			return exists();
		}

		std::size_t count() const noexcept {
			return m_values.size();
		}

		bool exists() const noexcept {
			return !m_values.empty();
		}

		template<class T>
		T as_type() const {
			return as_type_at<T>(0);
		}

		template<class T>
		T as_type(T &&default_val) const {
			return as_type_at<T>(0, std::forward<T>(default_val));
		}

		template<class T>
		T as_type_at(std::size_t idx) const {
			return as_type_at<T>(idx, false, T{});
		}

		template<class T>
		T as_type_at(std::size_t idx, T &&default_val) const {
			return as_type_at<T>(idx, true, std::forward<T>(default_val));
		}

		void print(std::size_t text_width) const override {
			std::stringstream ss;
			if (m_flag != 0)
				ss << "-" << m_flag << ", ";
			ss << "--" << m_name;
			if (m_type != Type::FLAG)
				ss << " " << str_to_upper(m_name);
			print_name(ss.str(), text_width);
		}

	private:
		friend class Argument_Parser;

		/**
		  * Convert string to all uppercase characters.
		 */
		static std::string str_to_upper(const std::string &str) {
			std::string rtn;
			std::transform(str.begin(), str.end(), std::back_inserter(rtn), toupper);
			return rtn;
		}

		char m_flag;
		Type m_type;
		std::vector<std::string> m_values;

		/* Private functions for Argument_Parser */

		char flag() const noexcept {
			return m_flag;
		}

		Type type() const noexcept {
			return m_type;
		}

		template<class T>
		T as_type_at(std::size_t idx, bool has_default, T &&default_val) const {
			if (exists())
				return parse_string_arg<T>(value(idx));
			if (has_default)
				return std::forward<T>(default_val);
			if (m_type == Type::FLAG)
				return parse_string_arg<T>("false");
			throw std::logic_error{lerrstr("no value given for '", m_name, "' and no default specified")};
		}

		std::string value(std::size_t idx) const {
			if (idx < m_values.size())
				return m_values[idx];
			throw std::out_of_range{lerrstr("index ", idx, " is out of range for '", m_name, "'")};
		}

		void set_flag(char flag) noexcept {
			m_flag = flag;
		}

		void set_values(std::vector<std::string> &&values) {
			m_values = std::move(values);
		}

	};

}

#endif /* CPPARSE_OPTIONAL_INFO_H_ */
