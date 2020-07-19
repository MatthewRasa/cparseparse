/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#include "argument-parser.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <string>

using Opt_Type = argparse::Argument_Parser::Optional_Type;

int main(int argc, char *argv[]) {
	argparse::Argument_Parser parser;
	parser.add_positional("string");
	parser.add_optional("-i", "--invert", Opt_Type::FLAG);
	parser.add_optional("-r", "--repeat", Opt_Type::SINGLE);
	parser.add_optional("-f", "--filter", Opt_Type::APPEND);
	parser.add_optional("--show-time", Opt_Type::FLAG);
	try {
		parser.parse_args(argc, argv);

		const auto tstart = std::chrono::high_resolution_clock::now();

		auto string_val = parser.arg<std::string>("string");

		for (std::size_t i = 0; i < parser.arg_count("filter"); ++i)
			string_val.erase(std::remove(string_val.begin(), string_val.end(), parser.arg_at<char>("filter", i)), string_val.end());

		if (parser.arg<bool>("invert"))
			std::sort(string_val.rbegin(), string_val.rend());
		else
			std::sort(string_val.begin(), string_val.end());

		const auto repeat = parser.arg<unsigned int>("repeat", 1);
		if (repeat > 0) {
			for (std::size_t i = 0; i < repeat; ++i)
				std::cout << string_val;
			std::cout << std::endl;
		}

		const auto tend = std::chrono::high_resolution_clock::now();
		if (parser.arg<bool>("show-time"))
			std::cout << "Completed in: " << std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count() << " us" << std::endl;

		return 0;
	} catch (const std::exception &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
