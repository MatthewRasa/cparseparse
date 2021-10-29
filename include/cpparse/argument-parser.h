/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_ARGUMENT_PARSER
#define CPPARSE_ARGUMENT_PARSER

#include "cpparse/optional-info.h"
#include "cpparse/positional-info.h"
#include "cpparse/util/compat.h"
#include <regex>
#include <unordered_map>

namespace cpparse {

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
			add_optional("-h", "--help", Optional_Info::Type::FLAG).help("display this help text");
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
		Optional_Info &add_optional(std::string long_name, Optional_Info::Type type = Optional_Info::Type::SINGLE) {
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
		Optional_Info &add_optional(std::string flag, String &&long_name, Optional_Info::Type type = Optional_Info::Type::SINGLE) {
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
			errstr_set_script_name(argv[0]);
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
			std::cout << "Usage: " << _scriptname;
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

			if (it->second->type() == Optional_Info::Type::FLAG) {
				if (repeated)
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				return "true";
			} else {
				if (++argi == argc)
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				std::string string_arg{argv[argi]};
				if (valid_option_name(string_arg))
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (it->second->type() != Optional_Info::Type::APPEND && repeated)
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

#endif /* CPPARSE_ARGUMENT_PARSER */
