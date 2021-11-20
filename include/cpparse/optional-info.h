/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_OPTIONAL_INFO_H_
#define CPPARSE_OPTIONAL_INFO_H_

#include "cpparse/argument-info.h"
#include "cpparse/util/compat.h"
#include "cpparse/util/string-ops.h"
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

		/**
		 * Construct optional argument info.
		 *
		 * @param name  argument name
		 * @param type  optional argument type
		 */
		template<class String>
		explicit Optional_Info(String &&name, Type type) noexcept(noexcept(Argument_Info{std::forward<String>(name)}))
				: Argument_Info{std::forward<String>(name)},
				  m_flag{NO_FLAG},
				  m_type{type} { }

		/**
		 * Implicit conversion to bool.
		 *
		 * Equivalent to calling exists().
		 */
		operator bool() const noexcept {
			return exists();
		}

		/**
		 * @return true if a flag is associated with this argument, or false otherwise
		 */
		bool has_flag() const noexcept {
			return m_flag != NO_FLAG;
		}

		/**
		 * @return the flag associated with this optional argument
		 */
		char flag() const noexcept {
			return m_flag;
		}

		/**
		 * @return the optional argument type
		 */
		Type type() const noexcept {
			return m_type;
		}

		/**
		 * @return the number of values given for the argument.
		 */
		std::size_t count() const noexcept {
			return m_values.size();
		}

		/**
		 * @return true if the argument was specified by the user, or false otherwise.
		 */
		bool exists() const noexcept {
			return !m_values.empty();
		}

		/**
		 * Retrieve the argument as a value of type T.
		 *
		 * @tparam T  type to retrieve the argument as
		 * @return the argument as a value of type T
		 */
		template<class T>
		T as_type() const {
			return as_type_at<T>(0);
		}

		/**
		 * Retrieve the argument as a value of type T.
		 *
		 * If the argument was not specified by the user, the given default value is used.
		 *
		 * @tparam T           type to retrieve the argument as
		 * @param default_val  default value to use if the argument was not specified by the user
		 * @return the argument as a value of type T
		 */
		template<class T>
		T as_type(T &&default_val) const {
			return as_type_at<T>(0, std::forward<T>(default_val));
		}

		/**
		 * Retrieve the argument at the given index as a value of type T.
		 *
		 * @tparam T   type to retrieve the argument as
		 * @param idx  index at which to retrieve value
		 * @return the argument as a value of type T
		 */
		template<class T>
		T as_type_at(std::size_t idx) const {
			return as_type_at<T, false>(idx, T{});
		}

		/**
		 * Retrieve the argument at the given index as a value of type T.
		 *
		 * If the argument was not specified by the user, the given default value is used.
		 *
		 * @tparam T           type to retrieve the argument as
		 * @param idx          index at which to retrieve value
		 * @param default_val  default value to use if the argument was not specified by the user
		 * @return the argument as a value of type T
		 */
		template<class T>
		T as_type_at(std::size_t idx, T &&default_val) const {
			return as_type_at<T, true>(idx, std::forward<T>(default_val));
		}

		/**
		 * Print argument description.
		 *
		 * @param text_width  text width spacing
		 * @param out         output stream
		 */
		void print(std::size_t text_width, std::ostream &out = std::cout) const {
			std::stringstream ss;
			if (has_flag())
				ss << "-" << m_flag << ", ";
			ss << "--" << m_name;
			if (m_type != Type::FLAG)
				ss << " " << str_to_upper(m_name);
			print_help(ss.str(), text_width, out);
		}

	private:
		friend class Argument_Parser;

		/** Special constant to indicate that there is no associated flag */
		static constexpr char NO_FLAG{0};

		char m_flag;
		Type m_type;
		std::vector<std::string> m_values;

		/**
		 * Retrieve the argument at the given index as a value of type T.
		 *
		 * If the argument was not specified by the user but a default value was given, the default value is used.
		 *
		 * @tparam T            type to retrieve the argument as
		 * @tparam has_default  compile-time boolean indicating whether a default value was given
		 * @param idx           index at which to retrieve value
		 * @param default_val   default value to use if the argument was not specified by the user
		 * @return the argument as a value of type T
		 * @throw std::logic_error  if no value was provided for the argument
		 */
		template<class T, bool has_default>
		T as_type_at(std::size_t idx, T &&default_val) const {
			if (exists())
				return parse_as_type<T>(value(idx));
			IF_CONSTEXPR (has_default)
				return std::forward<T>(default_val);
			if (m_type == Type::FLAG)
				return parse_as_type<T>("false");
			throw std::logic_error{lerrstr("no value given for '", m_name, "' and no default specified")};
		}

		/**
		 * Retrieve the argument value at the given index.
		 *
		 * @return the argument value as a string
		 * @throw std::out_of_range  if the index is out-of-range for the argument
		 */
		std::string value(std::size_t idx) const {
			if (idx >= m_values.size())
				throw std::out_of_range{lerrstr("index ", idx, " is out of range for '", m_name, "'")};
			return m_values[idx];
		}

		/* Private functions for Argument_Parser */

		/**
		 * Set the flag associated with this optional argument.
		 *
		 * @param flag  flag character
		 */
		void set_flag(char flag) noexcept {
			m_flag = flag;
		}

		/**
		 * Set the values for this optional argument.
		 *
		 * @param values  vector of values
		 */
		void set_values(std::vector<std::string> &&values) noexcept(std::is_nothrow_move_assignable<std::vector<std::string>>::value) {
			m_values = std::move(values);
		}

	};

}

#endif /* CPPARSE_OPTIONAL_INFO_H_ */
