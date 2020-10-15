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
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <vector>
#include <unordered_map>

namespace argparse {

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
		enum class Optional_Type {
			FLAG, SINGLE, APPEND
		};

		/**
		 * Positional info object storing information about a positional argument.
		 * Provides functions for setting additional argument properties.
		 */
		class Positional_Info {
		public:

			Positional_Info(const Positional_Info &) = delete;
			Positional_Info(Positional_Info &&) = delete;
			Positional_Info &operator=(const Positional_Info &) = delete;
			Positional_Info &operator=(Positional_Info &&) = delete;

			/**
			 * Set the help text that accompanies this argument when displaying program help.
			 *
			 * @tparam String    string-like type that is convertible to @a std::string.
			 * @param help_text  help text string.
			 * @return a reference to this object.
			 */
			template<class String>
			Positional_Info &help(String &&help_text) {
				this->help_text = std::forward<String>(help_text);
				return *this;
			}

		private:
			friend class Argument_Parser;

			Positional_Info() = default;

			std::string help_text;
			std::string value;
		};

		/**
		 * Optional info object storing information about an optional argument.
		 * Provides functions for setting additional argument properties.
		 */
		class Optional_Info {
		public:

			Optional_Info(const Optional_Info &) = delete;
			Optional_Info(Optional_Info &&) = delete;
			Optional_Info &operator=(const Optional_Info &) = delete;
			Optional_Info &operator=(Optional_Info &&) = delete;

			/**
			 * Set the help text that accompanies this argument when displaying program help.
			 *
			 * @tparam String    string-like type that is convertible to @a std::string.
			 * @param help_text  help text string.
			 * @return a reference to this object.
			 */
			template<class String>
			Optional_Info &help(String &&help_text) {
				this->help_text = std::forward<String>(help_text);
				return *this;
			}

		private:
			friend class Argument_Parser;

			Optional_Info(Optional_Type type, std::string name) noexcept
					: flag{' '},
					  type{type},
					  name{std::move(name)} { }

			char flag;
			Optional_Type type;
			std::string name;
			std::vector<std::string> values;
			std::string help_text;
		};

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
		template<class String>
		Positional_Info &add_positional(String &&name) {
			if (!valid_positional_name(name))
				throw std::logic_error{lerrstr("invalid positional argument name '", name, "'")};
			if (m_positional_args.find(name) != m_positional_args.end())
				throw std::logic_error{lerrstr("duplicate positional argument name '", name, "'")};
			if (m_optional_args.find(name) != m_optional_args.end())
				throw std::logic_error{lerrstr("positional argument name conflicts with optional argument reference name '", name, "'")};
			auto it = m_positional_args.emplace(
					std::forward<String>(name), std::unique_ptr<Positional_Info>{new Positional_Info{}}).first;
			m_positional_order.push_back(it->first);
			return *it->second;
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
			if (m_optional_args.find(formatted_name) != m_optional_args.end())
				throw std::logic_error{lerrstr("duplicate optional argument name '", formatted_name, "'")};
			if (m_positional_args.find(formatted_name) != m_positional_args.end())
				throw std::logic_error{lerrstr("optional argument reference name conflicts with positional argument name '", formatted_name, "'")};
			auto it = m_optional_args.emplace(formatted_name,
					std::unique_ptr<Optional_Info>{new Optional_Info{type, formatted_name}}).first;
			m_optional_order.push_back(it->second->name);
			return *it->second;
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
			if (formatted_name.empty())
				throw std::logic_error{lerrstr("invalid flag name '", flag, "'")};
			if (m_flags.find(formatted_name[0]) != m_flags.end())
				throw std::logic_error{lerrstr("duplicate flag name '", flag, "'")};
			auto &optional = add_optional(std::forward<String>(long_name), std::move(type));
			optional.flag = formatted_name[0];
			m_flags.emplace(optional.flag, optional.name).first;
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
			m_scriptname = argv[0];

			std::vector<const char *> positional_args;
			positional_args.reserve(argc - 1);
			std::unordered_map<std::string, std::vector<std::string>> optional_values;

			for (int argi = 1; argi < argc; ++argi) {
				std::string string_arg{argv[argi]};
				if (valid_option_name(string_arg))
					argi = parse_optional_arg(optional_values, string_arg, argi, argc, argv);
				else
					positional_args.push_back(argv[argi]);
			}
			if (positional_args.size() < m_positional_order.size())
				throw std::runtime_error{errstr("requires positional argument '", m_positional_order[positional_args.size()], "'")};

			std::size_t pos_idx;
			for (pos_idx = 0; pos_idx < m_positional_order.size(); ++pos_idx)
				m_positional_args[m_positional_order[pos_idx]]->value = positional_args[pos_idx];
			for (const auto &pair : optional_values)
				m_optional_args[pair.first]->values = std::move(pair.second);

			argc = positional_args.size() - pos_idx + 1;
			for ( ; pos_idx < positional_args.size(); ++pos_idx)
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
			const auto opt_it = m_optional_args.find(name);
			if (opt_it == m_optional_args.end())
				throw std::logic_error{lerrstr("no optional argument by the name '", name, "'")};
			return !opt_it->second->values.empty();
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
			const auto opt_it = m_optional_args.find(name);
			if (opt_it == m_optional_args.end())
				throw std::logic_error{lerrstr("no optional argument by the name '", name, "'")};
			return opt_it->second->values.size();
		}

		/**
		 * Print usage text to stdout.
		 */
		void print_usage() const {
			std::cout << "Usage: " << m_scriptname;
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
				for (const auto &positional : m_positional_order) {
					std::cout << "  ";
					std::cout << std::left << std::setw(18) << positional << m_positional_args.at(positional)->help_text << std::endl;
				}
			}
			if (!m_optional_args.empty()) {
				std::cout << std::endl << "Options:" << std::endl;
				for (const auto &optional : m_optional_order) {
					const auto &info = m_optional_args.at(optional);
					std::stringstream ss;
					ss << "  ";
					if (info->flag != ' ')
						ss << "-" << info->flag << ", ";
					ss << "--" << optional;
					if (info->type != Optional_Type::FLAG)
						ss << " " << str_to_upper(optional);
					std::cout << std::left << std::setw(30) << ss.str() << info->help_text << std::endl;
				}
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
		 * std::stoull overload to prevent unsigned integer wrap-around.
		 */
		static unsigned long long stoull(const std::string &value) {
			if (value.find('-') != std::string::npos)
				throw std::out_of_range{""};
			return std::stoull(value);
		}

		/**
		 * Convert string to all uppercase characters.
		 */
		static std::string str_to_upper(const std::string &str) {
			std::string rtn;
			std::transform(str.begin(), str.end(), std::back_inserter(rtn), toupper);
			return rtn;
		}

		std::string m_scriptname;
		std::vector<std::string> m_positional_order;
		std::unordered_map<std::string, std::unique_ptr<Positional_Info>> m_positional_args;
		std::vector<std::string> m_optional_order;
		std::unordered_map<std::string, std::unique_ptr<Optional_Info>> m_optional_args;
		std::unordered_map<char, std::string> m_flags;

		/**
		 * Check whether the positional argument name is valid.
		 */
		bool valid_positional_name(const std::string &name) const {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("\\w[a-zA-Z0-9_-]*"));
			return !m.empty();
		}

		/**
		 * Check whether the optional argument name is valid.
		 */
		bool valid_option_name(const std::string &name) {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("-([a-zA-Z_]|-?[a-zA-Z_][a-zA-Z0-9_-]+)"));
			return !m.empty();
		}

		/**
		 * Format the option name by removing the any leading '-' characters.
		 */
		std::string format_option_name(const std::string &name) const {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("--?([a-zA-Z_][a-zA-Z0-9_-]+)"));
			return m.size() < 2 ? "" : m[1].str();
		}

		/**
		 * Format the flag name by removing the leading '-' character.
		 */
		std::string format_flag_name(const std::string &name) const {
			std::cmatch m;
			std::regex_match(name.c_str(), m, std::regex("-([a-zA-Z_])"));
			return m.size() < 2 ? "" : m[1].str();
		}

		/**
		 * Parse optional argument from the list of user-provided arguments.
		 */
		std::size_t parse_optional_arg(std::unordered_map<std::string, std::vector<std::string>> &optional_values,
				const std::string &option_name, int argi, int argc, char const **argv) {
			const auto formatted_option_name = lookup_formatted_option_name(option_name);
			if (formatted_option_name == "help") {
				print_help();
				std::exit(0);
			}

			auto it = m_optional_args.find(formatted_option_name);
			if (it == m_optional_args.end())
				throw std::runtime_error{errstr("invalid option '", option_name, "', pass --help to display possible options")};
			auto &values = optional_values[formatted_option_name];
			if (it->second->type == Optional_Type::FLAG) {
				if (!values.empty())
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				values.push_back("true");
			} else {
				if (++argi == argc)
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				std::string string_val{argv[argi]};
				if (valid_option_name(string_val))
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (it->second->type != Optional_Type::APPEND && !values.empty())
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				values.push_back(std::move(string_val));
			}
			return argi;
		}

		/**
		 * Look up the option name as either a flag or option argument and
		 * return the formatted option name.
		 */
		std::string lookup_formatted_option_name(const std::string &option_name) const {
			const auto flag_name = format_flag_name(option_name);
			if (!flag_name.empty()) {
				const auto flag_it = m_flags.find(flag_name[0]);
				if (flag_it == m_flags.end())
					throw std::runtime_error{errstr("invalid flag '", option_name, "', pass --help to display possible options")};
				return flag_it->second;
			}
			return format_option_name(option_name);
		}

		/**
		 * Retrieve the value for the argument at the specified index with the
		 * given default value.
		 */
		template<class T>
		T arg_at(const std::string &name, std::size_t idx, bool has_default, T &&default_val) const {
			std::string value;
			const auto opt_it = m_optional_args.find(name);
			if (opt_it != m_optional_args.cend()) {
				if ((value = optional_arg(opt_it, idx)).empty()) {
					if (!has_default)
						throw std::logic_error{lerrstr("no value given for '", name, "' and no default specified")};
					return default_val;
				}
			} else {
				const auto it = m_positional_args.find(name);
				if (it == m_positional_args.cend())
					throw std::logic_error{lerrstr("no argument by the name '", name, "'")};
				value = it->second->value;
			}
			return parse_string_arg<T>(name, value);
		}

		/**
		 * Retrieve the value of the (possibly) user-supplied optional argument.
		 */
		std::string optional_arg(decltype(m_optional_args)::const_iterator it, std::size_t idx) const {
			if (idx < it->second->values.size())
				return it->second->values[idx];
			else if (it->second->values.empty())
				return it->second->type == Optional_Type::FLAG ? "false" : "";
			else
				throw std::out_of_range{lerrstr("index ", idx, " is out of range for '", it->first, "'")};
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
		typename std::enable_if<std::is_same<T, char>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			if (value.size() != 1)
				throw std::runtime_error{errstr("'", name, "' must be a single character")};
			return value[0];
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, bool>::value && std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return stoull(value); });
		}
		template<class T>
		typename std::enable_if<!std::is_same<T, char>::value && std::is_integral<T>::value && std::is_signed<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return std::stoll(value); });
		}
		template<class T>
		typename std::enable_if<std::is_floating_point<T>::value, T>::type parse_string_arg(const std::string &name, const std::string &value) const {
			return parse_numeric_arg<T>(name, value, [](const std::string &value) { return std::stold(value); });
		}
		template<class T>
		typename std::enable_if<std::is_same<T, std::string>::value, T>::type parse_string_arg(const std::string &, const std::string &value) const {
			return value;
		}

		/**
		 * Parse the string argument as a numeric of type T.
		 */
		template<class T, class Convert_Func>
		T parse_numeric_arg(const std::string &name, const std::string &value, Convert_Func &&convert_func) const {
			try {
				const auto n_value = convert_func(value);
				if (n_value < std::numeric_limits<T>::lowest() || n_value > std::numeric_limits<T>::max())
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
