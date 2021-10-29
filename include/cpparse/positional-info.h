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

}

#endif /* CPPARSE_POSITIONAL_INFO_H_ */
