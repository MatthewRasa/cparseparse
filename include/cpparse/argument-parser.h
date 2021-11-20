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
#include <unordered_set>

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
		 * Retrieve the program description.
		 *
		 * @return the program description string
		 */
		const std::string &description() const noexcept {
			return m_description;
		}

		/**
		 * Set the program description.
		 *
		 * @tparam String      string-like type that is convertible to @a std::string
		 * @param description  description string
		 */
		template<class String>
		void set_description(String &&description) noexcept {
			m_description = std::forward<String>(description);
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
		 * the values. After the call, @a argc and @a argv are updated to refer to any
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
			parse_args(argc, const_cast<const char **&>(argv));
		}

		/**
		 * @see parse_args()
		 */
		void parse_args(int &argc, const char **&argv) {
			errstr_set_script_name(argv[0]);
			remove_matched(assign_user_args(argc, argv), argc, argv);
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
			return arg_at<T, false>(name, idx, T{});
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
			return arg_at<T, true>(name, idx, std::forward<T>(default_val));
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
		 *
		 * @param out  output stream [default: @a std::cout]
		 */
		void print_usage(std::ostream &out = std::cout) const {
			out << "Usage: " << _scriptname;
			if (!m_optional_args.empty())
				out << " [options]";
			for (const auto &positional : m_positional_order)
				out << " <" << positional << ">";
			out << std::endl;
		}

		/**
		 * Print help text to stdout.
		 *
		 * @param out  output stream [default: @a std::cout]
		 */
		void print_help(std::ostream &out = std::cout) const {
			print_usage(out);
			if (!m_description.empty())
				out << std::endl << "  " << m_description << std::endl;
			if (!m_positional_order.empty()) {
				out << std::endl << "Positional arguments:" << std::endl;
				for (const auto &positional : m_positional_order)
					m_positional_args.at(positional)->print(20, out);
			}
			if (!m_optional_args.empty()) {
				out << std::endl << "Options:" << std::endl;
				for (const auto &optional : m_optional_order)
					m_optional_args.at(optional)->print(30, out);
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

		/**
		 * Parsed argument token structure.
		 */
		struct Opt_Token {
			enum class Type { OPT_NAME, OPT_VAL, FLAG };
			Type type;
			std::string value;
		};

		/** Map structure to store matched optional arguments */
		using Matched_Opts = std::unordered_map<std::string, std::vector<std::string>>;

		std::string m_description;
		std::vector<std::string> m_positional_order;
		std::unordered_map<std::string, std::unique_ptr<Positional_Info>> m_positional_args;
		std::vector<std::string> m_optional_order;
		std::unordered_map<std::string, std::unique_ptr<Optional_Info>> m_optional_args;
		std::unordered_map<char, std::string> m_flags;

		/**
		 * Parse the command-line arguments into tokens and match/assign them to their
		 * corresponding parameters.
		 */
		std::vector<const char *> assign_user_args(int argc, const char **argv) {
			std::vector<const char *> pos_args;
			Matched_Opts opt_args;
			std::tie(pos_args, opt_args) = match_args(argc, argv);
			assign_matched_opt(std::move(opt_args));
			return assign_matched_pos(std::move(pos_args));
		}

		/**
		 * Assign the matched positional arguments to their corresponding parameters.
		 */
		std::vector<const char *> assign_matched_pos(const std::vector<const char *> &pos_args) {
			for (std::size_t i = 0; i < m_positional_order.size(); ++i)
				m_positional_args[m_positional_order[i]]->set_value(pos_args[i]);
			return std::vector<const char *>(pos_args.data() + m_positional_order.size(), pos_args.data() + pos_args.size());
		}

		/**
		 * Assign the matched optional arguments to their corresponding parameters.
		 */
		void assign_matched_opt(Matched_Opts &&optional_args) {
			for (auto &&pair : std::move(optional_args))
				m_optional_args[pair.first]->set_values(std::move(pair.second));
		}

		/**
		 * Parse the command-line arguments into tokens and match them to their
		 * corresponding parameters.
		 */
		std::pair<std::vector<const char *>, Matched_Opts> match_args(int argc, const char **argv) const {
			std::vector<const char *> pos_args;
			std::vector<Opt_Token> opt_args;
			std::tie(pos_args, opt_args) = tokenize_args(argc, argv);
			return std::make_pair(pos_args, match_opt_args(opt_args));
		}

		/**
		 * Match the tokenized arguments to their corresponding optional parameters.
		 */
		Matched_Opts match_opt_args(const std::vector<Opt_Token> &arg_tokens) const {
			auto optional_args = init_matched_opt_args(arg_tokens);
			const std::string *p_last_optional;
			for (const auto &token : arg_tokens) {
				switch (token.type) {
				case Opt_Token::Type::OPT_NAME:
					p_last_optional = &token.value;
					break;
				case Opt_Token::Type::OPT_VAL:
					optional_args[*p_last_optional].push_back(std::move(token.value));
					break;
				case Opt_Token::Type::FLAG:
					optional_args[token.value].push_back("true");
					break;
				}
			}
			return optional_args;
		}

		/**
		 * Initialize the map used to store matched optional arguments.
		 */
		Matched_Opts init_matched_opt_args(const std::vector<Opt_Token> &arg_tokens) const {
			Matched_Opts optional_args;
			const auto optional_counts = count_opt_args(arg_tokens);
			optional_args.reserve(optional_counts.size());
			for (const auto &pair : optional_counts)
				optional_args[pair.first].reserve(pair.second);
			return optional_args;
		}

		/**
		 * Count the number of optional arguments from the token list.
		 */
		std::unordered_map<std::string, std::size_t> count_opt_args(const std::vector<Opt_Token> &tokens) const {
			std::unordered_map<std::string, std::size_t> optional_counts;
			for (const auto &token : tokens) {
				if (token.type == Opt_Token::Type::OPT_NAME || token.type == Opt_Token::Type::FLAG)
					++optional_counts[token.value];
			}
			return optional_counts;
		}

		/**
		 * Parse the command-line arguments into tokens that provide information for the
		 * next parsing step.
		 */
		std::pair<std::vector<const char *>, std::vector<Opt_Token>> tokenize_args(int argc, const char **argv) const {
			const std::vector<const char *> user_args(argv + 1, argv + argc);
			std::vector<const char *> pos_args;
			pos_args.reserve(user_args.size());
			std::vector<Opt_Token> opt_tokens;
			opt_tokens.reserve(user_args.size());

			std::unordered_set<std::string> optional_names;
			for (auto it = user_args.begin(); it != user_args.end(); ++it) {
				auto optional_name = lookup_formatted_option_name(*it);
				if (optional_name.empty()) {
					pos_args.push_back(*it);
				} else {
					if (prematch_optional_arg(optional_name, optional_names.find(optional_name) != optional_names.end(), *std::next(it), *(user_args.end()))) {
						opt_tokens.push_back(Opt_Token{Opt_Token::Type::OPT_NAME, optional_name});
						opt_tokens.push_back(Opt_Token{Opt_Token::Type::OPT_VAL, *(++it)});
					} else {
						opt_tokens.push_back(Opt_Token{Opt_Token::Type::FLAG, optional_name});
					}
					optional_names.insert(optional_name);
				}
			}
			if (pos_args.size() < m_positional_order.size())
				throw std::runtime_error{errstr("requires positional argument '", m_positional_order[pos_args.size()], "'")};

			return std::make_pair(pos_args, opt_tokens);
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
		 * Perform initial validation/matching on the optional argument.
		 */
		bool prematch_optional_arg(const std::string &option_name, bool repeated, const char *next_arg, const char *args_end) const {
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
				return false;
			} else {
				if (next_arg == args_end)
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (valid_option_name(next_arg))
					throw std::runtime_error{errstr("'", option_name, "' requires a value")};
				if (it->second->type() != Optional_Info::Type::APPEND && repeated)
					throw std::runtime_error{errstr("'", option_name, "' should only be specified once")};
				return true;
			}
		}

		/**
		 * Update the command-line argument variables to refer to any extra positional
		 * arguments that have not been matched.
		 */
		void remove_matched(const std::vector<const char *> &extra_positional_args, int &argc, const char **&argv) const {
			argc = extra_positional_args.size() + 1;
			for (std::size_t i = 0; i < extra_positional_args.size(); ++i)
				argv[i + 1] = extra_positional_args[i];
		}

		/**
		 * Retrieve the value for the argument at the specified index with the given
		 * default value.
		 */
		template<class T, bool has_default>
		T arg_at(const std::string &name, std::size_t idx, T &&default_val) const {
			const auto opt_it = m_optional_args.find(name);
			if (opt_it != m_optional_args.cend())
				return opt_it->second->as_type_at<T, has_default>(idx, std::forward<T>(default_val));

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
