/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPARSEPARSE_ARGUMENT_INFO_H_
#define CPARSEPARSE_ARGUMENT_INFO_H_

#include "cparseparse/util/errstr.h"
#include "cparseparse/util/string-ops.h"
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace cpparse {

	/**
	 * Argument info object storing information about a generic argument.
	 */
	template<class Argument_Type>
	class Argument_Info {
	public:

		/**
		 * Construct argument info.
		 *
		 * @param name  argument name
		 */
		template<class String>
		explicit Argument_Info(String &&name) noexcept(std::is_nothrow_constructible<std::string, String &&>::value)
				: m_name{std::forward<String>(name)} { }

		/**
		 * Default virtual destructor.
		 */
		virtual ~Argument_Info() = default;

		/* Disable copy/move operations */
		Argument_Info(const Argument_Info &) = delete;
		Argument_Info(Argument_Info &&) = delete;
		Argument_Info &operator=(const Argument_Info &) = delete;
		Argument_Info &operator=(Argument_Info &&) = delete;

		/**
		 * @return the argument name
		 */
		const std::string &name() const noexcept {
			return m_name;
		}

		/**
		 * @return the argument help text
		 */
		const std::string &help() const noexcept {
			return m_help_text;
		}

		/**
		 * Set the help text that accompanies this argument when displaying program help.
		 *
		 * @tparam String    string-like type that is convertible to @a std::string
		 * @param help_text  help text string
		 * @return a reference to this object
		 */
		template<class String>
		Argument_Type &help(String &&help_text) noexcept(std::is_nothrow_assignable<std::string, String &&>::value) {
			m_help_text = std::forward<String>(help_text);
			return reinterpret_cast<Argument_Type &>(*this);
		}

	protected:

		std::string m_name;
		std::string m_help_text;

		/**
		 * Print the argument name and help text.
		 *
		 * @param text_width  text width spacing
		 * @param out         output stream
		 */
		void print_help(std::size_t text_width, std::ostream &out = std::cout) const {
			print_help(m_name, text_width, out);
		}

		/**
		 * Print the argument name and help text.
		 *
		 * @param name        argument name
		 * @param text_width  text width spacing
		 * @param out         output stream
		 */
		void print_help(const std::string &name, std::size_t text_width, std::ostream &out = std::cout) const {
			out << "  " << std::left << std::setw(text_width - 2) << name << m_help_text << std::endl;
		}

		/**
		 * Parse the argument as type T.
		 *
		 * Valid choices for T are booleans, unsigned/signed integer types, floating point types, and std::string.
		 *
		 * @tparam T     type to parse the argument as
		 * @param value  the argument value as a string
		 * @return the argument parsed as type T
		 */
		template<class T>
		typename std::enable_if<std::is_same<T, bool>::value, T>::type parse_as_type(const std::string &value) const {
			if (value == "true" || value == "yes" || value == "on")
				return true;
			else if (value == "false" || value == "no" || value == "off")
				return false;
			throw std::runtime_error{errstr("'", m_name, "' must be one of: 'true', 'false', 'yes', 'no', 'on', 'off'")};
		}
		template<class T>
		typename std::enable_if<std::is_same<T, char>::value, T>::type parse_as_type(const std::string &value) const {
			if (value.size() != 1)
				throw std::runtime_error{errstr("'", m_name, "' must be a single character")};
			return value[0];
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type parse_as_type(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return cpparse::stoull(value); });
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, char>::value && std::is_integral<T>::value && std::is_signed<T>::value, T>::type parse_as_type(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return std::stoll(value); });
		}
		template<class T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type parse_as_type(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return std::stold(value); });
		}
		template<class T>
		typename std::enable_if<std::is_same<T, std::string>::value, T>::type parse_as_type(std::string value) const noexcept {
			return value;
		}

	private:

		/**
		 * Parse the string argument as a numeric of type T.
		 */
		template<class T, class Convert_Func>
		T parse_numeric_arg(const std::string &value, Convert_Func &&convert_func) const {
			try {
				const auto n_value = convert_func(value);
				if (n_value < std::numeric_limits<T>::lowest() || n_value > std::numeric_limits<T>::max())
					throw std::out_of_range{""};
				return n_value;
			} catch (const std::invalid_argument &ex) {
				throw std::runtime_error{errstr("'", m_name, "' must be of integral type")};
			} catch (const std::out_of_range &ex) {
				throw std::runtime_error{errstr("'", m_name, "' must be in range [", std::numeric_limits<T>::min(), ",", std::numeric_limits<T>::max(), "]")};
			}
		}

	};

}

#endif /* CPARSEPARSE_ARGUMENT_INFO_H_ */
