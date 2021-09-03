/*
 * Author: Matthew Rasa
 * E-mail: matt@raztech.com
 * GitHub: https://github.com/MatthewRasa
 */

#include "argument-parser.h"
#include <catch2/catch.hpp>

using namespace Catch::Matchers;

using Opt_Type = argparse::Optional_Type;

TEST_CASE("Argument_Parser add_positional()") {
	argparse::Argument_Parser parser;
	parser.add_positional("pos0");
	parser.add_positional("pos1");
	parser.add_optional("--opt0");
	REQUIRE_THROWS_WITH(parser.add_positional("pos0"), Contains("duplicate positional argument name"));
	REQUIRE_THROWS_WITH(parser.add_positional("-pos0"), Contains("invalid positional argument name"));
	REQUIRE_THROWS_WITH(parser.add_positional("opt0"), Contains("positional argument name conflicts with optional argument reference name"));
}

TEST_CASE("Argument_Parser add_optional()") {
	argparse::Argument_Parser parser;
	parser.add_positional("pos0");
	REQUIRE_THROWS_WITH(parser.add_optional("opt1", Opt_Type::SINGLE), Contains("invalid optional argument name"));
	REQUIRE_THROWS_WITH(parser.add_optional("a", "opt1", Opt_Type::SINGLE), Contains("invalid flag name"));

	parser.add_optional("--opt1", Opt_Type::SINGLE);
	REQUIRE_THROWS_WITH(parser.add_optional("--opt1", Opt_Type::SINGLE), Contains("duplicate optional argument name"));
	REQUIRE_THROWS_WITH(parser.add_optional("--pos0", Opt_Type::SINGLE), Contains("optional argument reference name conflicts with positional argument name"));
	parser.add_optional("-a", "--opt2", Opt_Type::FLAG);
	REQUIRE_THROWS_WITH(parser.add_optional("-a", "--opt2", Opt_Type::SINGLE), Contains("duplicate flag name"));
	REQUIRE_THROWS_WITH(parser.add_optional("-b", "--opt2", Opt_Type::APPEND), Contains("duplicate optional argument name"));
	parser.add_optional("-b", "--opt3", Opt_Type::APPEND);;
}

TEST_CASE("Argument_Parser parse_args()") {
	argparse::Argument_Parser parser;
	auto call_parse_args = [&parser](std::vector<const char *> args) {
		int argc = args.size();
		auto argv = args.data();
		try {
			parser.parse_args(argc, argv);
		} catch (...) {
			REQUIRE(argc == static_cast<int>(args.size()));
			throw;
		}
		REQUIRE(argv[0] == args.at(0));
		return std::vector<std::string>{argv + 1, argv + argc};
	};

	SECTION("Positional") {
		parser.add_positional("param1");
		parser.add_positional("param2");
		std::vector<const char *> args{"test-program", "arg1"};
		REQUIRE_THROWS_WITH(call_parse_args(args), "test-program: requires positional argument 'param2'");

		args.push_back("arg2");
		REQUIRE(call_parse_args(args).empty());

		args.push_back("arg3");
		const auto extra_args = call_parse_args(args);
		REQUIRE(extra_args.size() == 1);
		REQUIRE(extra_args.at(0) == "arg3");
	}
	SECTION("Optional") {
		SECTION("Invalid option") {
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "--opt0"}), EndsWith(", pass --help to display possible options"));
		}
		SECTION("Flag argument") {
			parser.add_optional("-o", "--opt0", Opt_Type::FLAG);
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o", "--opt0"}), EndsWith("should only be specified once"));
			const auto args = call_parse_args({"test-program", "-o", "extra1"});
			REQUIRE(args.size() == 1);
			REQUIRE(args.at(0) == "extra1");
		}
		SECTION("Single argument") {
			parser.add_optional("-o", "--opt0", Opt_Type::SINGLE);
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o", "abc", "--opt0", "abc"}), EndsWith("should only be specified once"));
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o"}), EndsWith("requires a value"));
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o", "-a"}), EndsWith("requires a value"));
			const auto args = call_parse_args({"test-program", "-o", "a", "extra1"});
			REQUIRE(args.size() == 1);
			REQUIRE(args.at(0) == "extra1");
		}
		SECTION("Append argument") {
			parser.add_optional("-o", "--opt0", Opt_Type::APPEND);
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o"}), EndsWith("requires a value"));
			REQUIRE_THROWS_WITH(call_parse_args({"test-program", "-o", "-a"}), EndsWith("requires a value"));
			const auto args = call_parse_args({"test-program", "-o", "abc", "--opt0", "def", "extra1", "-o", "ghi", "extra2"});
			REQUIRE(args.size() == 2);
			REQUIRE(args.at(0) == "extra1");
			REQUIRE(args.at(1) == "extra2");
		}
	}
}

TEST_CASE("Argument_Parser arg()") {
	argparse::Argument_Parser parser;
	parser.add_positional("barg");
	parser.add_positional("carg");
	parser.add_positional("uiarg");
	parser.add_positional("siarg");
	parser.add_positional("darg");
	parser.add_positional("sarg");
	parser.add_optional("--flag", Opt_Type::FLAG);
	parser.add_optional("--other-flag", Opt_Type::FLAG);
	parser.add_optional("--single", Opt_Type::SINGLE);
	parser.add_optional("--default-single", Opt_Type::SINGLE);
	parser.add_optional("--append", Opt_Type::APPEND);
	parser.add_optional("--default-append", Opt_Type::APPEND);

	std::vector<const char *> args{"test-program", "true", "r", "77", "-5", "-9.5", "abc123", "--flag", "--single", "27", "--append", "-30", "--append", "-31", "--append", "-32"};
	int argc = args.size();
	auto argv = args.data();
	parser.parse_args(argc, argv);

	SECTION("Positional") {
		REQUIRE_THROWS_WITH(parser.arg<std::string>("unknown"), Contains("no argument by the name"));
		REQUIRE(parser.arg<uint>("uiarg") == 77);
		REQUIRE(parser.arg<bool>("barg") == true);
	}
	SECTION("Optional") {
		SECTION("Flag argument") {
			REQUIRE(parser.has_arg("flag"));
			REQUIRE(parser.arg_count("flag") == 1);
			REQUIRE(parser.arg<bool>("flag"));
			REQUIRE(parser.arg<bool>("flag", false));
			REQUIRE_THROWS_WITH(parser.arg_at<bool>("flag", 1), EndsWith("index 1 is out of range for 'flag'"));

			REQUIRE(!parser.has_arg("other-flag"));
			REQUIRE(parser.arg_count("other-flag") == 0);
			REQUIRE(!parser.arg<bool>("other-flag"));
			REQUIRE(parser.arg<bool>("other-flag", true));
		}
		SECTION("Single argument") {
			REQUIRE(parser.has_arg("single"));
			REQUIRE(parser.arg_count("single") == 1);
			REQUIRE(parser.arg<int>("single") == 27);
			REQUIRE(parser.arg<int>("single", 24) == 27);
			REQUIRE_THROWS_WITH(parser.arg_at<int>("single", 1), EndsWith("index 1 is out of range for 'single'"));

			REQUIRE(!parser.has_arg("default-single"));
			REQUIRE(parser.arg_count("default-single") == 0);
			REQUIRE(parser.arg<int>("default-single", 24) == 24);
			REQUIRE_THROWS_WITH(parser.arg<int>("default-single"), EndsWith("no value given for 'default-single' and no default specified"));
		}
		SECTION("Append argument") {
			REQUIRE(parser.has_arg("append"));
			REQUIRE(parser.arg_count("append") == 3);
			REQUIRE(parser.arg<int>("append") == -30);
			REQUIRE(parser.arg_at<int>("append", 0) == -30);
			REQUIRE(parser.arg_at<int>("append", 1) == -31);
			REQUIRE(parser.arg_at<int>("append", 2) == -32);
			REQUIRE_THROWS_WITH(parser.arg_at<int>("append", 3), EndsWith("index 3 is out of range for 'append'"));

			REQUIRE(!parser.has_arg("default-append"));
			REQUIRE(parser.arg_count("default-append") == 0);
			REQUIRE(parser.arg<int>("default-append", 25) == 25);
			REQUIRE_THROWS_WITH(parser.arg<int>("default-append"), EndsWith("no value given for 'default-append' and no default specified"));
		}
	}
	SECTION("Value parsing") {
		REQUIRE(parser.arg<bool>("barg") == true);
		REQUIRE_THROWS_WITH(parser.arg<char>("barg"), EndsWith("must be a single character"));
		REQUIRE_THROWS_WITH(parser.arg<uint>("barg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<int>("barg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<double>("barg"), EndsWith("must be of integral type"));
		REQUIRE(parser.arg<std::string>("barg") == "true");

		REQUIRE_THROWS_WITH(parser.arg<bool>("carg"), EndsWith("must be either 'true' or 'false'"));
		REQUIRE(parser.arg<char>("carg") == 'r');
		REQUIRE_THROWS_WITH(parser.arg<uint>("carg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<int>("carg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<double>("carg"), EndsWith("must be of integral type"));
		REQUIRE(parser.arg<std::string>("carg") == "r");

		REQUIRE_THROWS_WITH(parser.arg<bool>("uiarg"), EndsWith("must be either 'true' or 'false'"));
		REQUIRE_THROWS_WITH(parser.arg<char>("uiarg"), EndsWith("must be a single character"));
		REQUIRE(parser.arg<uint>("uiarg") == 77);
		REQUIRE(parser.arg<int>("uiarg") == 77);
		REQUIRE(parser.arg<double>("uiarg") == 77);
		REQUIRE(parser.arg<std::string>("uiarg") == "77");

		REQUIRE_THROWS_WITH(parser.arg<bool>("siarg"), EndsWith("must be either 'true' or 'false'"));
		REQUIRE_THROWS_WITH(parser.arg<char>("siarg"), EndsWith("must be a single character"));
		REQUIRE_THROWS_WITH(parser.arg<uint>("siarg"), Contains("must be in range"));
		REQUIRE(parser.arg<int>("siarg") == -5);
		REQUIRE(parser.arg<double>("siarg") == -5);
		REQUIRE(parser.arg<std::string>("siarg") == "-5");

		REQUIRE_THROWS_WITH(parser.arg<bool>("darg"), EndsWith("must be either 'true' or 'false'"));
		REQUIRE_THROWS_WITH(parser.arg<char>("darg"), EndsWith("must be a single character"));
		REQUIRE_THROWS_WITH(parser.arg<uint>("darg"), Contains("must be in range"));
		REQUIRE(parser.arg<int>("darg") == -9);
		REQUIRE(parser.arg<double>("darg") == -9.5);
		REQUIRE(parser.arg<std::string>("darg") == "-9.5");

		REQUIRE_THROWS_WITH(parser.arg<bool>("sarg"), EndsWith("must be either 'true' or 'false'"));
		REQUIRE_THROWS_WITH(parser.arg<char>("sarg"), EndsWith("must be a single character"));
		REQUIRE_THROWS_WITH(parser.arg<uint>("sarg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<int>("sarg"), EndsWith("must be of integral type"));
		REQUIRE_THROWS_WITH(parser.arg<double>("sarg"), EndsWith("must be of integral type"));
		REQUIRE(parser.arg<std::string>("sarg") == "abc123");
	}
}
