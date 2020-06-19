/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPP_ARGPARSE_ARGUMENT_PARSER
#define CPP_ARGPARSE_ARGUMENT_PARSER

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>
#include <unordered_map>

namespace cpp_argparse {

	/**
	 * Command-line argument parser.
	 *
	 * Usage steps:
	 * 1. Add argument definitions using add_positional() and add_optional().
	 * 2. Pass the user-supplied command-line arguments to parse_args().
	 * 3. Retrieve each argument by name using arg().
	 */
	class Argument_Parser {
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
		 *         appending values to an argument list.
		 */
		enum class Optional_Type {
			FLAG, SINGLE, APPEND
		};

		/**
		 * Define a positional argument with the given name.
		 *
		 * The assigned name is referenced in the help text and can be passed to arg()
		 * to retrieve the argument value.
		 *
		 * @tparam String  string-like type that is convertible to @a std::string.
		 * @param name     positional argument name.
		 */
		template<class String>
		void add_positional(String &&name) {
			m_positional_args.push_back(std::make_pair(std::forward<String>(name), ""));
		}

		/**
		 * Define an optional argument with the given "long" name and default value.
		 *
		 * The reference name becomes @a long_name without the leading dashes. The
		 * reference name is referenced in the help text and can be passed to arg() to
		 * retrieve the argument value.
		 *
		 * @tparam Default     default value type, defaults to @a std::string.
		 * @param long_name    argument "long" name (starts with @p '-' or @p '--'
		 *                     followed by a non-digit character).
		 * @param default_val  value used when the user does not provide a value for the
		 *                     argument.
		 * @return The reference name.
		 * @throw std::logic_error  If @a long_name is not in the correct format or is a
		 *                          duplicate.
		 */
		template<class Default = std::string>
		std::string add_optional(std::string long_name, Optional_Type type = Optional_Type::SINGLE, Default &&default_val = "") {
			auto formatted_name = format_option_name(long_name);
			if (formatted_name.empty())
				throw std::logic_error{lerrstr("invalid optional argument name: ", long_name)};
			if (m_optional_args.find(formatted_name) != m_optional_args.end())
				throw std::logic_error{lerrstr("duplicate optional argument name '", formatted_name, "'")};
			return m_optional_args.emplace(std::move(formatted_name),
					Optional_Info{type, type == Optional_Type::FLAG ? "false" : arg_to_string(std::forward<Default>(default_val)), {}}).first->first;
		}

		/**
		 * Define an optional argument with the given flag name, "long" name and default
		 * value.
		 *
		 * The reference name becomes @a long_name without the leading dashes. The
		 * reference name is referenced in the help text and can be passed to arg() to
		 * retrieve the argument value.
		 *
		 * @tparam String      string-like type that is convertible to @a std::string.
		 * @tparam Default     default value type, defaults to @a std::string.
		 * @param flag         flag name (@p '-' followed by a single non-digit
		 *                     character).
		 * @param long_name    argument "long" name (starts with @p '-' or @p '--'
		 *                     followed by a non-digit character).
		 * @param default_val  value used when the user does not provide a value for the
		 *                     argument.
		 * @return The reference name.
		 * @throw std::logic_error  If either @a flag or @a long_name are not in the
		 *                          correct format or are duplicates.
		 */
		template<class String, class Default = std::string>
		std::string add_optional(std::string flag, String &&long_name, Optional_Type type = Optional_Type::SINGLE, Default &&default_val = "") {
			if (flag.size() != 2 || flag[0] != '-' || !valid_option_char(flag[1]))
				throw std::logic_error{lerrstr("invalid flag name '", flag, "'")};
			if (m_flags.find(flag[1]) != m_flags.end())
				throw std::logic_error{lerrstr("duplicate flag name '", flag, "'")};
			return m_flags.emplace(flag[1],
					add_optional(std::forward<String>(long_name), std::move(type), std::forward<Default>(default_val))).first->second;
		}

		/**
		 * Parse the user-provided command-line arguments and match the values to the
		 * program arguments.
		 *
		 * This function must be called before arg() can be used to retrieve the values.
		 *
		 * @param argc  command-line argument count
		 * @param argv  command-line argument strings
		 * @throw std::runtime_error  If a positional argument is missing, a value for
		 *                            an optional argument is missing, or an optional
		 *                            argument value was supplied an incorrect number of
		 *                            times.
		 */
		void parse_args(int argc, char *argv[]) {
			m_scriptname = argv[0];

			std::size_t pos_idx = 0;
			for (std::size_t argi = 1; argi < argc; ++argi) {
				std::string string_arg{argv[argi]};
				auto formatted_name = format_option_name(string_arg);
				if (!formatted_name.empty()) {
					argi = parse_optional_arg(string_arg, formatted_name, argi, argc, argv);
				} else if (pos_idx < m_positional_args.size()) {
					m_positional_args[pos_idx++].second = std::move(string_arg);
				}
			}
			if (pos_idx < m_positional_args.size())
				throw std::runtime_error{errstr("requires positional argument '", m_positional_args[pos_idx].first, "'")};
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
			const auto opt_it = m_optional_args.find(name);
			if (opt_it == m_optional_args.end())
				throw std::logic_error{lerrstr("no optional argument by the name '", name, "'")};
			return !opt_it->second.values.empty();
		}

		/**
		 * Retrieve the value of the (possibly) user-supplied argument.
		 *
		 * If the user supplied a value for the specified positional or optional
		 * argument, it will be parsed as type @a T and returned. If the user did not
		 * supply a value but a default value was specified in add_positional() or
		 * add_optional(), the default value will be parsed as type @a T and returned.
		 * Otherwise if no user-supplied or default value is available, an exception is
		 * thrown.
		 *
		 * @tparam T    type to parse argument as.
		 * @param name  positional or optional argument name. For optional arguments,
		 *              the reference name (the value returned from add_optional())
		 *              should be used.
		 * @param idx   index at which to retrieve argument; valid only for optional
		 *              append-type arguments.
		 * @return The user-supplied or default value parsed as type @a T.
		 * @throw std::out_of_range   If the index is out of range for the specified
		 *                            optional argument.
		 * @throw std::logic_error    If no positional or optional argument with the
		 *                            specified name exists, or no value is available.
		 * @throw std::runtime_error  If the argument cannot be parsed as type @a T.
		 */
		template<class T>
		T arg(const std::string &name, std::size_t idx = 0) const {
			std::string value;
			const auto opt_it = m_optional_args.find(name);
			if (opt_it != m_optional_args.cend()) {
				value = optional_arg(opt_it, idx);
			} else {
				const auto it = std::find_if(m_positional_args.cbegin(), m_positional_args.cend(),
					[&name](const std::pair<std::string, std::string> &pair) { return pair.first == name; });
				if (it == m_positional_args.cend())
					throw std::logic_error{lerrstr("no argument by the name '", name, "'")};
				value = it->second;
			}
			return parse_string_arg<T>(name, value);
		}

		/**
		 * Get the number of values provided for the specified optional append-type
		 * argument.
		 *
		 * @param name  optional append-type argument reference name.
		 * @return the number of provided values.
		 * @throw std::logic_error  If no optional argument with the specified name
		 *                          exists.
		 */
		std::size_t arg_count(const std::string &name) const {
			const auto opt_it = m_optional_args.find(name);
			if (opt_it == m_optional_args.end())
				throw std::logic_error{lerrstr("no optional argument by the name '", name, "'")};
			return opt_it->second.values.size();
		}

	private:

		/**
		 * Determine if the character is a valid start for an option name.
		 */
		static bool valid_option_char(char c) noexcept {
			return !('0' <= c && c <= '9') && c != '-';
		}

		/**
		 * std::stoull overload to prevent unsigned integer wrap-around.
		 */
		static unsigned long long stoull(const std::string &value) {
			if (value.find('-') != std::string::npos)
				throw std::out_of_range{""};
			return std::stoull(value);
		}

		/**
		 * Optional info struct.
		 */
		struct Optional_Info {
			Optional_Type type;
			std::string default_val;
			std::vector<std::string> values;
		};

		std::string m_scriptname;
		std::vector<std::pair<std::string, std::string>> m_positional_args;
		std::unordered_map<std::string, Optional_Info> m_optional_args;
		std::unordered_map<char, std::string> m_flags;

		/**
		 * Parse optional argument from the list of user-provided arguments.
		 */
		std::size_t parse_optional_arg(const std::string &option_name, const std::string &formatted_name, std::size_t argi, int argc, char *argv[]) {
			auto it = m_optional_args.find(formatted_name);
			if (it == m_optional_args.end() && option_name.size() == 2) {
				auto flag_it = m_flags.find(formatted_name[0]);
				if (flag_it != m_flags.end())
					it = m_optional_args.find(flag_it->second);
			}
			if (it == m_optional_args.end())
				throw std::runtime_error{errstr("invalid option '", option_name, "', pass --help to display possible options")};
			if (it->second.type == Optional_Type::FLAG) {
				if (!it->second.values.empty())
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				it->second.values.push_back("true");
			} else {
				if (argi + 1 == argc)
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				std::string string_val{argv[argi + 1]};
				if (!format_option_name(string_val).empty())
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (it->second.type != Optional_Type::APPEND && !it->second.values.empty())
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				it->second.values.push_back(std::move(string_val));
			}
			return argi;
		}

		/**
		 * Retrieve the value of the (possibly) user-supplied optional argument.
		 */
		std::string optional_arg(decltype(m_optional_args)::const_iterator it, std::size_t idx) const {
			if (it->second.values.empty()) {
				if (it->second.default_val.empty())
					throw std::logic_error{lerrstr("no value given for '", it->first, "' and no default specified")};
				return it->second.default_val;
			} else if (idx < it->second.values.size()) {
				return it->second.values[idx];
			} else {
				throw std::out_of_range{lerrstr("index ", idx, " is out of range for '", it->first, "'")};
			}
		}

		/**
		 * Parse the string argument as type T.
		 *
		 * Valid choices for T are booleans, unsigned/signed integer types, floating point types, and std::string.
		 */
		template<class T>
		typename std::enable_if<std::is_same<T, bool>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			if (value == "true")
				return true;
			else if (value == "false")
				return false;
			throw std::runtime_error{errstr("'", name, "' must be either 'true' or 'false'")};
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return stoull(value); });
		}
		template<class T>
		typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return std::stoll(value); });
		}
		template<class T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return std::stold(value); });
		}
		template<class T>
		typename std::enable_if<std::is_same<T, std::string>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return value;
		}

		/**
		 * Parse the string argument as a numeric of type T.
		 */
		template<class T, class Convert_Func>
		T parse_numeric_arg(const std::string &name, const std::string &value, Convert_Func &&convert_func) const {
			try {
				const auto n_value = convert_func(value);
				if (n_value < std::numeric_limits<T>::min() || n_value > std::numeric_limits<T>::max())
					throw std::out_of_range{""};
				return n_value;
			} catch (const std::invalid_argument &ex) {
				throw std::runtime_error{errstr("'", name, "' must be of integral type")};
			} catch (const std::out_of_range &ex) {
				throw std::runtime_error{errstr("'", name, "' must be in range [", std::numeric_limits<T>::min(), ",", std::numeric_limits<T>::max(), "]")};
			}
		}

		/**
		 * Convert the argument of type T to std::string.
		 */
		template<class T>
		typename std::enable_if<std::is_convertible<T, std::string>::value, std::string>::type arg_to_string(T &&arg) const {
			return arg;
		}
		template<class T>
		typename std::enable_if<!std::is_convertible<T, std::string>::value, std::string>::type arg_to_string(T &&arg) const {
			return std::to_string(arg);
		}

		/**
		 * Format the option name by removing the any leading '-' characters.
		 */
		std::string format_option_name(const std::string &option_name) const noexcept {
			if (option_name.size() < 2 || option_name[0] != '-')
				return "";
			std::size_t start_idx = 1;
			if (option_name[start_idx] == '-') {
				if (option_name.size() < 3)
					return "";
				++start_idx;
			}
			return valid_option_char(option_name[start_idx]) ? option_name.substr(start_idx) : "";
		}

		/**
		 * Concatenate words into a runtime error string.
		 */
		template<class ...Args>
		std::string errstr(Args&&... args) const {
			std::stringstream ss;
			_errstr(ss, m_scriptname, ": ", std::forward<Args>(args)...);
			return ss.str();
		}

		/**
		 * Concatenate words into a logic error string.
		 */
		template<class ...Args>
		std::string lerrstr(Args&&... args) const {
			std::stringstream ss;
			_errstr(ss, "Argument_Parser: ", std::forward<Args>(args)...);
			return ss.str();
		}

		/**
		 * Recursive helpers for errstr()/lerrstr().
		 */
		template<class Arg, class ...Args>
		void _errstr(std::stringstream &ss, Arg &&arg, Args&&... args) const {
			_errstr(ss, std::forward<Arg>(arg));
			_errstr(ss, std::forward<Args>(args)...);
		}
		template<class Arg>
		void _errstr(std::stringstream &ss, Arg &&arg) const {
			ss << std::forward<Arg>(arg);
		}

	};

}

#endif /* CPP_ARGPARSE_ARGUMENT_PARSER */
