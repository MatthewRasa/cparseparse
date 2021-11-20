/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#include "cparseparse/argument-parser.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <string>

/** Optional argument type alias */
using Opt_Type = cpparse::Optional_Info::Type;

/**
 * Example program to perform a string sort with a variety of options.
 */
void run_program(std::string string_val, bool invert, unsigned int repeat, const std::vector<char> &filters, bool show_time) {
	using Clock = std::chrono::system_clock;
	const auto tstart = Clock::now();

	for (auto filter_char : filters)
		string_val.erase(std::remove(string_val.begin(), string_val.end(), filter_char), string_val.end());

	if (invert)
		std::sort(string_val.rbegin(), string_val.rend());
	else
		std::sort(string_val.begin(), string_val.end());

	if (repeat > 0) {
		for (std::size_t i = 0; i < repeat; ++i)
			std::cout << string_val;
		std::cout << std::endl;
	}

	const auto tend = Clock::now();
	if (show_time)
		std::cout << "Completed in: " << std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count() << " us" << std::endl;
}

/**
 * Sample main function showing argument parser usage.
 */
int main(int argc, char *argv[]) {
	// Define command-line parameters
	cpparse::Argument_Parser parser;
	parser.set_description("Sort the provided string with a variety of options");
	parser.add_positional("string").help("string to sort");
	parser.add_optional("-i", "--invert", Opt_Type::FLAG).help("invert sort to put string in reverse order");
	parser.add_optional("-r", "--repeat", Opt_Type::SINGLE).help("print REPEAT instances of the string [default: 1]");
	parser.add_optional("-f", "--filter", Opt_Type::APPEND).help("filter out the given character (may be specified more than once)");
	parser.add_optional("--show-time", Opt_Type::FLAG).help("display the time it took to complete the sort");

	// Attempt to parse the user-provided arguments
	try {
		parser.parse_args(argc, argv);
		// After parsing, matched arguments are removed from argc/argv such that
		// only unmatched positional arguments remain.
	} catch (const std::runtime_error &ex) {
		// If parsing fails, report the exception and print the program usage
		std::cerr << ex.what() << std::endl;
		parser.print_usage();
		return 1;
	}

	// Extract parsed arguments and run the program!
	run_program(parser.arg<std::string>("string"),
				parser.arg<bool>("invert"),
				parser.arg<unsigned int>("repeat", 1),
				parser.args<char>("filter"),
				parser.arg<bool>("show-time"));
	return 0;
}
