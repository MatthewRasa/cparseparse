/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#ifndef CPPARSE_POSITIONAL_INFO_H_
#define CPPARSE_POSITIONAL_INFO_H_

#include "cpparse/argument-info.h"

namespace cpparse {

	/**
	 * Positional info object storing information about a positional argument.
	 *
	 * Provides functions for setting additional argument properties.
	 */
	class Positional_Info : public Argument_Info<Positional_Info> {
	public:

		/**
		 * Construct positional argument info.
		 *
		 * @param name  argument name
		 */
		template<class String>
		explicit Positional_Info(String &&name) noexcept(noexcept(Argument_Info{std::forward<String>(name)}))
				: Argument_Info{std::forward<String>(name)} { }

		/**
		 * Retrieve the argument as a value of type T.
		 *
		 * @tparam T  type to retrieve the argument as
		 * @return the argument as a value of type T
		 */
		template<class T>
		T as_type() const noexcept(noexcept(parse_as_type<T>(std::string{}))) {
			return parse_as_type<T>(m_value);
		}

		/**
		 * Print argument description.
		 *
		 * @param text_width  text width spacing
		 * @param out         output stream
		 */
		void print(std::size_t text_width, std::ostream &out = std::cout) const {
			print_help(text_width, out);
		}

	private:
		friend class Argument_Parser;

		std::string m_value;

		/* Private functions for Argument_Parser */

		template<class String>
		void set_value(String &&value) noexcept(std::is_nothrow_assignable<decltype(m_value), String &&>::value) {
			m_value = std::forward<String>(value);
		}

	};

}

#endif /* CPPARSE_POSITIONAL_INFO_H_ */
