/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPP_ARGPARSE_ARGUMENT_PARSER
#define CPP_ARGPARSE_ARGUMENT_PARSER

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace argparse {

	std::string g_scriptname;

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
		_errstr(ss, g_scriptname, ": ", std::forward<Args>(args)...);
		return ss.str();
	}

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
	enum class Optional_Type { FLAG, SINGLE, APPEND };

	/**
	 * Argument info object storing information about a generic argument.
	 */
	template<class Argument_Type>
	class Argument_Info {
	public:

		/* Default virtual destructor */
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
		Argument_Type &help(String &&help_text) {
			m_help_text = std::forward<String>(help_text);
			return reinterpret_cast<Argument_Type &>(*this);
		}

		/**
		 * Print argument description.
		 *
		 * @param text_width  text width spacing
		 */
		virtual void print(std::size_t text_width) const {
			print_name(m_name, text_width);
		}

	protected:

		std::string m_name;
		std::string m_help_text;

		/**
		 * Construct argument info.
		 *
		 * @param name  argument name
		 */
		template<class String>
		explicit Argument_Info(String &&name)
				: m_name{std::forward<String>(name)} { }

		/**
		 * Print the argument using the specified name
		 *
		 * @param name        argument name
		 * @param text_width  text_width spacing
		 */
		void print_name(const std::string &name, std::size_t text_width) const {
			std::cout << "  " << std::left << std::setw(text_width - 2) << name << m_help_text << std::endl;
		}

		/**
		 * Parse the string argument as type T.
		 *
		 * Valid choices for T are booleans, unsigned/signed integer types, floating point types, and std::string.
		 */
		template<class T>
		typename std::enable_if<std::is_same<T, bool>::value, T>::type parse_string_arg(const std::string &value) const {
			if (value == "true")
				return true;
			else if (value == "false")
				return false;
			throw std::runtime_error{errstr("'", m_name, "' must be either 'true' or 'false'")};
		}
		template<class T>
		typename std::enable_if<std::is_same<T, char>::value, T>::type parse_string_arg(const std::string &value) const {
			if (value.size() != 1)
				throw std::runtime_error{errstr("'", m_name, "' must be a single character")};
			return value[0];
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type parse_string_arg(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return stoull(value); });
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, char>::value && std::is_integral<T>::value && std::is_signed<T>::value, T>::type parse_string_arg(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return std::stoll(value); });
		}
		template<class T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type parse_string_arg(const std::string &value) const {
			return parse_numeric_arg<T>(value, [](const std::string &value) { return std::stold(value); });
		}
		template<class T>
		typename std::enable_if<std::is_same<T, std::string>::value, T>::type parse_string_arg(const std::string &value) const {
			return value;
		}

	private:

		/**
		 * std::stoull overload to prevent unsigned integer wrap-around.
		 */
		static unsigned long long stoull(const std::string &value) {
			if (value.find('-') != std::string::npos)
				throw std::out_of_range{""};
			return std::stoull(value);
		}

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

	/**
	 * Positional info object storing information about a positional argument.
	 *
	 * Provides functions for setting additional argument properties.
	 */
	class Positional_Info : public Argument_Info<Positional_Info> {
	public:

		template<class String>
		explicit Positional_Info(String &&name)
				: Argument_Info{std::forward<String>(name)} { }

		template<class T>
		T as_type() const {
			return parse_string_arg<T>(m_value);
		}

	private:
		friend class Argument_Parser;

		std::string m_value;

		/* Private functions for Argument_Parser */

		template<class String>
		void set_value(String &&value) {
			m_value = std::forward<String>(value);
		}

	};

	/**
	 * Optional info object storing information about an optional argument.
	 *
	 * Provides functions for setting additional argument properties.
	 */
	class Optional_Info : public Argument_Info<Optional_Info> {
	public:

		template<class String>
		explicit Optional_Info(String &&name, Optional_Type type) noexcept
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
			if (m_type != Optional_Type::FLAG)
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
		Optional_Type m_type;
		std::vector<std::string> m_values;

		/* Private functions for Argument_Parser */

		char flag() const noexcept {
			return m_flag;
		}

		Optional_Type type() const noexcept {
			return m_type;
		}

		template<class T>
		T as_type_at(std::size_t idx, bool has_default, T &&default_val) const {
			if (exists())
				return parse_string_arg<T>(value(idx));
			if (has_default)
				return std::forward<T>(default_val);
			if (m_type == Optional_Type::FLAG)
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

	/**
	 * Command-line argument parser.
	 *
	 * Usage steps:
	 * 1. Add argument definitions using add_positional() and add_optional().
	 * 2. Pass the user-supplied command-line arguments to parse_args().
	 * 3. Retrieve each argument by name using arg() and arg_at().
	 */
	class Argument_Parser {
	public:

		Argument_Parser() {
			add_optional("-h", "--help", Optional_Type::FLAG).help("display this help text");
		}

		/**
		 * Define a positional argument with the given name.
		 *
		 * The assigned name is referenced in the help text and can be passed to arg()
		 * to retrieve the argument value.
		 *
		 * @tparam String  string-like type that is convertible to @a std::string.
		 * @param name     positional argument name.
		 * @return a reference to this argument's Positional_Info object that can be
		 *         used to set additional argument properties.
		 * @throw std::logic_error  If @a name is not in the correct format, it is a
		 *                          duplicate, or it conflicts with an optional argument
		 *                          name.
		 */
		Positional_Info &add_positional(std::string name) {
			if (!valid_positional_name(name))
				throw std::logic_error{lerrstr("invalid positional argument name '", name, "'")};
			if (m_optional_args.find(name) != m_optional_args.end())
				throw std::logic_error{lerrstr("positional argument name conflicts with optional argument reference name '", name, "'")};
			const auto pair = m_positional_args.emplace(name, make_unique<Positional_Info>(name));
			if (!pair.second)
				throw std::logic_error{lerrstr("duplicate positional argument name '", name, "'")};
			m_positional_order.push_back(std::move(name));
			return *pair.first->second;
		}

		/**
		 * Define an optional argument with the given "long" name and type.
		 *
		 * The reference name becomes @a long_name without the leading dashes. The
		 * reference name is referenced in the help text and can be passed to arg() or
		 * arg_at() to retrieve the argument value.
		 *
		 * @param long_name  argument "long" name (starts with @p '-' or @p '--'
		 *                   followed by a non-digit character).
		 * @param type       optional argument type.
		 * @return a reference to this argument's Optional_Info object that can be used
		 *         to set additional argument properties.
		 * @throw std::logic_error  If @a long_name is not in the correct format, it is
		 *                          a duplicate, or it conflicts with an optional
		 *                          argument name.
		 */
		Optional_Info &add_optional(std::string long_name, Optional_Type type = Optional_Type::SINGLE) {
			auto formatted_name = format_option_name(long_name);
			if (formatted_name.empty())
				throw std::logic_error{lerrstr("invalid optional argument name: ", long_name)};
			if (m_positional_args.find(formatted_name) != m_positional_args.end())
				throw std::logic_error{lerrstr("optional argument reference name conflicts with positional argument name '", formatted_name, "'")};
			const auto pair = m_optional_args.emplace(formatted_name, make_unique<Optional_Info>(formatted_name, type));
			if (!pair.second)
				throw std::logic_error{lerrstr("duplicate optional argument name '", formatted_name, "'")};
			m_optional_order.push_back(std::move(formatted_name));
			return *pair.first->second;
		}

		/**
		 * Define an optional argument with the given flag name, "long" name, and type.
		 *
		 * The reference name becomes @a long_name without the leading dashes. The
		 * reference name is referenced in the help text and can be passed to arg() or
		 * arg_at() to retrieve the argument value.
		 *
		 * @tparam String    string-like type that is convertible to @a std::string.
		 * @param flag       flag name (@p '-' followed by a single non-digit
		 *                   character).
		 * @param long_name  argument "long" name (starts with @p '-' or @p '--'
		 *                   followed by a non-digit character).
		 * @param type       optional argument type.
		 * @return a reference to this argument's Optional_Info object that can be used
		 *         to set additional argument properties.
		 * @throw std::logic_error  If either @a flag or @a long_name are not in the
		 *                          correct format or are duplicates.
		 */
		template<class String>
		Optional_Info &add_optional(std::string flag, String &&long_name, Optional_Type type = Optional_Type::SINGLE) {
			const auto formatted_name = format_flag_name(flag);
			if (!formatted_name)
				throw std::logic_error{lerrstr("invalid flag name '", flag, "'")};
			if (m_flags.find(formatted_name) != m_flags.end())
				throw std::logic_error{lerrstr("duplicate flag name '", flag, "'")};
			auto &optional = add_optional(std::forward<String>(long_name), std::move(type));
			optional.set_flag(formatted_name);
			m_flags.emplace(optional.flag(), optional.name()).first;
			return optional;
		}

		/**
		 * Parse the user-provided command-line arguments and match the values to the
		 * program arguments.
		 *
		 * This function must be called before arg() or arg_at() can be used to retrieve
		 * the values. After the call, argc and argv are updated to refer to any
		 * remaining command-line arguments not matched by the registered arguments.
		 *
		 * @param argc  reference to command-line argument count
		 * @param argv  reference to command-line argument strings
		 * @throw std::runtime_error  If a positional argument is missing, a value for
		 *                            an optional argument is missing, or an optional
		 *                            argument value was supplied an incorrect number of
		 *                            times.
		 */
		void parse_args(int &argc, char **&argv) {
			parse_args(argc, const_cast<char const **&>(argv));
		}
		void parse_args(int &argc, char const **&argv) {
			g_scriptname = argv[0];
			std::vector<const char *> positional_args;
			std::unordered_map<std::string, std::vector<std::string>> optional_args;
			std::tie(positional_args, optional_args) = match_args(argc, argv);

			std::size_t pos_idx;
			for (pos_idx = 0; pos_idx < m_positional_order.size(); ++pos_idx)
				m_positional_args[m_positional_order[pos_idx]]->set_value(std::move(positional_args[pos_idx]));
			for (auto &&pair : std::move(optional_args))
				m_optional_args[pair.first]->set_values(std::move(pair.second));

			argc = positional_args.size() - pos_idx + 1;
			for (; pos_idx < positional_args.size(); ++pos_idx)
				argv[pos_idx - m_positional_order.size() + 1] = positional_args[pos_idx];
		}

		/**
		 * Determine whether the user has supplied a value for the specified optional
		 * argument.
		 *
		 * @param name  optional argument reference name.
		 *
		 * @return true if the value has been specified, or false otherwise.
		 * @throw std::logic_error  If no optional argument with the specified name
		 *                          exists.
		 */
		bool has_arg(const std::string &name) const {
			return lookup_optional(name).exists();
		}

		/**
		 * Retrieve the value of the user-supplied argument.
		 *
		 * If the user supplied a value for the specified positional or optional
		 * argument, it will be parsed as type @a T and returned. Otherwise, an
		 * exception is thrown.
		 *
		 * @tparam T    type to parse argument as.
		 * @param name  positional or optional argument name. For optional arguments,
		 *              the reference name (the value returned from add_optional())
		 *              should be used.
		 * @return The user-supplied value parsed as type @a T.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists, or no value is available.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		T arg(const std::string &name) const {
			return arg_at<T>(name, 0);
		}

		/**
		 * Retrieve the value of the possibly user-supplied argument.
		 *
		 * If the user supplied a value for the specified positional or optional
		 * argument, it will be parsed as type @a T and returned. Otherwise, @a
		 * default_val will be parsed as type @a T and returned.
		 *
		 * @tparam T           type to parse argument as.
		 * @param name         positional or optional argument name. For optional
		 *                     arguments, the reference name (the value returned from
		 *                     add_optional()) should be used.
		 * @param default_val  value used when the user does not provide a value for the
		 *                     argument.
		 * @return The user-supplied or default value parsed as type @a T.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		T arg(const std::string &name, T &&default_val) const {
			return arg_at<T>(name, 0, std::forward<T>(default_val));
		}

		/**
		 * Retrieve the list of values that the user supplied for the given argument.
		 *
		 * All of the user-supplied values for the specified positional or optional
		 * argument will be parsed as type @a T and returned in a vector. If no values
		 * were supplied, an empty vector is returned.
		 *
		 * @tparam T    type to parse arguments as.
		 * @param name  positional or optional argument name. For optional arguments,
		 *              the reference name (the value returned from add_optional())
		 *              should be used.
		 * @return A vector containing the user-supplied values parsed as type @a T.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		std::vector<T> args(const std::string &name) const {
			const auto count = arg_count(name);

			std::vector<T> values;
			values.reserve(count);
			for (std::size_t i = 0; i < count; ++i)
				values.push_back(arg_at<T>(name, i));
			return values;
		}

		/**
		 * Retrieve the value of the user-supplied argument at the specified index.
		 *
		 * If the user supplied a value for the specified positional or optional
		 * argument at the given index, it will be parsed as type @a T and returned.
		 * Otherwise, an exception is thrown.
		 *
		 * @tparam T    type to parse argument as.
		 * @param name  positional or optional argument name. For optional arguments,
		 *              the reference name (the value returned from add_optional())
		 *              should be used.
		 * @param idx   index at which to retrieve argument.
		 * @return The user-supplied value parsed as type @a T.
		 * @throw std::out_of_range   If the index is out of range for the specified
		 *                            optional argument.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists, or no value is available.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		T arg_at(const std::string &name, std::size_t idx) const {
			return arg_at<T>(name, idx, false, T{});
		}

		/**
		 * Retrieve the value of the user-supplied argument at the specified index.
		 *
		 * If the user supplied a value for the specified positional or optional
		 * argument at the given index, it will be parsed as type @a T and returned.
		 * Otherwise, an exception is thrown.
		 *
		 * @tparam T           type to parse argument as.
		 * @param name         positional or optional argument name. For optional
		 *                     arguments, the reference name (the value returned from
		 *                     add_optional()) should be used.
		 * @param idx          index at which to retrieve argument.
		 * @param default_val  value used when the user does not provide a value for the
		 *                     argument.
		 * @return The user-supplied or default value parsed as type @a T.
		 * @throw std::out_of_range   If the index is out of range for the specified
		 *                            optional argument.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		T arg_at(const std::string &name, std::size_t idx, T &&default_val) const {
			return arg_at<T>(name, idx, true, std::forward<T>(default_val));
		}

		/**
		 * Get the number of values provided for the specified optional append-type
		 * argument.
		 *
		 * @param name  optional append-type argument reference name.
		 * @return The number of provided values.
		 * @throw std::logic_error  If no optional argument with the specified name
		 *                          exists.
		 */
		std::size_t arg_count(const std::string &name) const {
			return lookup_optional(name).count();
		}

		/**
		 * Print usage text to stdout.
		 */
		void print_usage() const {
			std::cout << "Usage: " << g_scriptname;
			if (!m_optional_args.empty())
				std::cout << " [options]";
			for (const auto &positional : m_positional_order)
				std::cout << " <" << positional << ">";
			std::cout << std::endl;
		}

		/**
		 * Print help text to stdout.
		 */
		void print_help() const {
			print_usage();
			if (!m_positional_order.empty()) {
				std::cout << std::endl << "Positional arguments:" << std::endl;
				for (const auto &positional : m_positional_order)
					m_positional_args.at(positional)->print(20);
			}
			if (!m_optional_args.empty()) {
				std::cout << std::endl << "Options:" << std::endl;
				for (const auto &optional : m_optional_order)
					m_optional_args.at(optional)->print(30);
			}
		}

	private:

		/**
		 * Determine if the character is a valid start for an option name.
		 */
		static bool valid_option_char(char c) noexcept {
			return !('0' <= c && c <= '9') && c != '-';
		}

		/**
		 * Check whether the positional argument name is valid.
		 */
		static bool valid_positional_name(const std::string &name) {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("\\w[a-zA-Z0-9_-]*"));
			return !m.empty();
		}

		/**
		 * Check whether the optional argument name is valid.
		 */
		static bool valid_option_name(const std::string &name) {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("-([a-zA-Z_]|-?[a-zA-Z_][a-zA-Z0-9_-]+)"));
			return !m.empty();
		}

		/**
		 * Format the option name by removing the any leading '-' characters.
		 */
		static std::string format_option_name(const std::string &name) {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("--?([a-zA-Z_][a-zA-Z0-9_-]+)"));
			return m.size() < 2 ? "" : m[1].str();
		}

		/**
		 * Format the flag name by removing the leading '-' character.
		 */
		static char format_flag_name(const std::string &name) {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("-([a-zA-Z_])"));
			return m.size() < 2 ? 0 : m[1].str()[0];
		}

		struct Arg_Info {
			const char *arg;
			std::string option_name;
			std::string option_value;
		};

		struct Counter {
			bool operator==(std::size_t val) const noexcept {
				return m_val == val;
			}
			bool operator!=(std::size_t val) const noexcept {
				return !(*this == val);
			}
			bool operator<(std::size_t val) const noexcept {
				return m_val < val;
			}
			bool operator<=(std::size_t val) const noexcept {
				return m_val <= val;
			}
			bool operator>(std::size_t val) const noexcept {
				return !(*this <= val);
			}
			bool operator>=(std::size_t val) const noexcept {
				return !(*this < val);
			}
			Counter &operator++() noexcept {
				++m_val;
				return *this;
			}
			std::size_t operator*() const noexcept {
				return m_val;
			}
			std::size_t m_val{0};
		};

		std::vector<std::string> m_positional_order;
		std::unordered_map<std::string, std::unique_ptr<Positional_Info>> m_positional_args;
		std::vector<std::string> m_optional_order;
		std::unordered_map<std::string, std::unique_ptr<Optional_Info>> m_optional_args;
		std::unordered_map<char, std::string> m_flags;

		/**
		 * Attempt to parse the user-provided command-line arguments.
		 *
		 * @param argc  command-line argument count
		 * @param argv  command-line argument strings
		 * @return the parsed positional and optional argument values.
		 * @throw std::runtime_error  If a positional argument is missing, a value for
		 *                            an optional argument is missing, or an optional
		 *                            argument value was supplied an incorrect number of
		 *                            times.
		 */
		std::pair<std::vector<const char *>, std::unordered_map<std::string, std::vector<std::string>>> match_args(
				int argc, const char **argv) const {
			std::vector<const char *> positional_args;
			std::unordered_map<std::string, std::vector<std::string>> optional_args;
			for (auto &&arg_info : prematch_args(argc, argv, positional_args, optional_args)) {
				if (arg_info.option_name.empty())
					positional_args.push_back(std::move(arg_info.arg));
				else
					optional_args[arg_info.option_name].push_back(std::move(arg_info.option_value));
			}
			return std::make_pair(std::move(positional_args), std::move(optional_args));
		}

		std::vector<Arg_Info> prematch_args(int argc, const char **argv,
				std::vector<const char *> &positional_args,
				std::unordered_map<std::string, std::vector<std::string>> &optional_args) const {
			std::vector<Arg_Info> arg_infos;
			arg_infos.reserve(argc - 1);
			std::size_t positional_count = 0;
			std::unordered_map<std::string, Counter> optional_occurs;
			for (int argi = 1; argi < argc; ++argi) {
				Arg_Info arg_info;
				arg_info.arg = argv[argi];
				arg_info.option_name = lookup_formatted_option_name(arg_info.arg);
				if (arg_info.option_name.empty())
					++positional_count;
				else
					arg_info.option_value = prematch_optional_arg(arg_info.option_name,
							++optional_occurs[arg_info.option_name] > 1, argi, argc, argv);
				arg_infos.push_back(std::move(arg_info));
			}
			if (positional_count < m_positional_order.size())
				throw std::runtime_error{errstr("requires positional argument '", m_positional_order[positional_count], "'")};

			positional_args.reserve(positional_count);
			for (const auto &pair : optional_occurs)
				optional_args[pair.first].reserve(*pair.second);
			return arg_infos;
		}

		/**
		 * Parse optional argument from the list of user-provided arguments.
		 */
		std::string prematch_optional_arg(const std::string &option_name, bool repeated, int &argi, int argc, const char **argv) const {
			if (option_name == "help") {
				print_help();
				std::exit(0);
			}

			auto it = m_optional_args.find(option_name);
			if (it == m_optional_args.end())
				throw std::runtime_error{errstr("invalid option '", option_name, "', pass --help to display possible options")};

			if (it->second->type() == Optional_Type::FLAG) {
				if (repeated)
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				return "true";
			} else {
				if (++argi == argc)
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				std::string string_arg{argv[argi]};
				if (valid_option_name(string_arg))
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (it->second->type() != Optional_Type::APPEND && repeated)
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				return string_arg;
			}
		}

		/**
		 * Look up the option name as either a flag or option argument and return the
		 * formatted option name.
		 */
		std::string lookup_formatted_option_name(const std::string &option_name) const {
			const auto flag_name = format_flag_name(option_name);
			if (flag_name) {
				const auto flag_it = m_flags.find(flag_name);
				if (flag_it == m_flags.end())
					throw std::runtime_error{errstr("invalid flag '", option_name, "', pass --help to display possible options")};
				return flag_it->second;
			}
			return format_option_name(option_name);
		}

		/**
		 * Retrieve the value for the argument at the specified index with the given
		 * default value.
		 */
		template<class T>
		T arg_at(const std::string &name, std::size_t idx, bool has_default, T &&default_val) const {
			const auto opt_it = m_optional_args.find(name);
			if (opt_it != m_optional_args.cend())
				return opt_it->second->as_type_at<T>(idx, has_default, std::forward<T>(default_val));

			const auto pos_it = m_positional_args.find(name);
			if (pos_it != m_positional_args.cend())
				return pos_it->second->as_type<T>();

			throw std::logic_error{lerrstr("no argument by the name '", name, "'")};
		}

		const Optional_Info &lookup_optional(const std::string &name) const {
			const auto opt_it = m_optional_args.find(name);
			if (opt_it == m_optional_args.end())
				throw std::logic_error{lerrstr("no optional argument by the name '", name, "'")};
			return *opt_it->second;
		}

		Optional_Info &lookup_optional(const std::string &name) {
			return const_cast<Optional_Info &>(static_cast<const Argument_Parser *>(this)->lookup_optional(name));
		}

	};

}

#endif /* CPP_ARGPARSE_ARGUMENT_PARSER */
