# CParseParse

A lightweight C++ command-line argument parsing utility.

## Table of Contents

* [Design and Features](#design-and-features)
* [Getting Started](#getting-started)
  * [Setup](#setup)
  * [Post Setup (Optional)](#post-setup-optional)
    * [Running Unit Tests](#running-unit-tests)
    * [Running the Sample Program](#running-the-sample-program)
* [Tutorial](#tutorial)
  * [Minimal Example](#minimal-example)
  * [Positional Arguments](#positional-arguments)
    * [Argument descriptions](#argument-descriptions)
  * [Optional Arguments](#optional-arguments)
    * [Single-Value Arguments](#single-value-arguments)
    * [Flag Arguments](#flag-arguments)
    * [Append-Style Arguments](#append-style-arguments)
  * [Overriding Default Help Behavior](#overriding-default-help-behavior)
* [API Reference](#api-reference)

## Design and Features

CParseParse is designed to be a lightweight C++ argument parsing solution with a few key attributes: 
* Employs efficient multiple-phase argument parsing
* Supports various types of command-line arguments
* Implemented as a template library for easy install and use
* Compatible with C++11/14/17/20
* Permissive [MIT License](LICENSE)

**Currently supported features**
* Positional arguments:  
  `$ ./my-program with some args`
* Optional single-value arguments (both short and long):  
  `$ ./my-program -l 1 --log-level 2`
* Optional boolean flag arguments:  
  `$ ./my-program --verbose`
* Optional append-style arguments:  
  `$ ./my-program --exclude-pattern dir/pat1* --exclude-pattern dir/pat2*`
* Configurable program description and per-argument help text
* Configurable handler for `-h/--help`
* Default values for omitted arguments

**Features planned for the future**
* Option to parse from configuration file instead of command-line
* Sub-parser sections

## Getting Started

### Setup

Start by cloning the repository:

```
git clone git@github.com:MatthewRasa/cparseparse.git
```

Then move into the project directory and install the headers via make:

```
make install
```

The setup is now complete! The installed files can be found under `<PREFIX>/cparseparse/` (the default `PREFIX` is `/usr/local`).

The project files can also be uninstalled via make:

```
make uninstall
```

You can continue with the quick-start by heading directly to the [Tutorial](#tutorial) section. Alternatively, you can proceed with the additional post-setup steps below.

### Post-Setup (Optional)

#### Running Unit Tests

The unit tests included in the repository are useful for verifying that everything is working properly. If you with to run the tests, [Catch2](https://github.com/catchorg/Catch2) is a required dependency.

The `run-tests` target will build and run the tests:

```
make run-tests
```

Catch2 will print a test report detailing the results. A successful run should yield output similar to below:

```
make -C test
make[1]: Entering directory '/home/matt/Projects/cparseparse/test'
make[1]: 'unit-tests' is up to date.
make[1]: Leaving directory '/home/matt/Projects/cparseparse/test'
./test/unit-tests
===============================================================================
All tests passed (128 assertions in 6 test cases)
```

#### Running the Sample Program

A sample program demonstrating use of CParseParse is included in the repository at [example/src/sort-string.cc](example/src/sort-string.cc).

The sample program can be built with the `example` target:

```
make example
```

Once built, the program executable will be placed under `example/`:

```
./example/sort-string --help
```

## Tutorial

This tutorial will walk through the features of CParseParse by writing a simple toy program. Installing CParseParse in the [Setup](#setup) section is a prerequisite.

We'll start by writing the canonical Hello World C++ program in a file called `hello.cc`:

```c++
#include <iostream>

int main(int argc, char *argv[]) {
	std::cout << "Hello World!" << std::endl;
	return 0;
}
```

Compile and run the program as usual. E.g. with gcc:

```
g++ -Wall -Wextra -std=c++20 -o hello hello.cc
./hello
--------------------OUTPUT--------------------
Hello World!
```

### Minimal Example

To begin using the argument parser, we'll modify the Hello World program to instantiate an `Argument_Parser` instance:

```c++
#include <cparseparse/argument-parser.h>  // Include header file
#include <iostream>

using namespace cpparse;  // Use the CParseParse namespace

int main(int argc, char *argv[]) {
	Argument_Parser parser{};  // Instantiate parser instance
	parser.set_description("Print 'Hello World!' to the console");  // Set the program description (optional)
	try {
		parser.parse_args(argc, argv);  // Parse the command-line arguments
		
		std::cout << "Hello World!" << std::endl;
	} catch (const std::runtime_error &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
```

While the argument parser is now present, no paramters have been specified. This means that when we compile and run the program as before with no arguments, we end up with the same result:

```
g++ -Wall -Wextra -std=c++20 -o hello hello.cc
./hello
--------------------OUTPUT--------------------
Hello World!
```

However, the `Argument_Parser` instance is configured with a `-h/--help` parameter by default, which we could pass as an argument. The default behavior in this case is to print the program help text and exit:

```
./hello -h
# OR
./hello --help
--------------------OUTPUT--------------------
Usage: ./hello [options]

  Print 'Hello World!' to the console

Options:
  -h, --help                  display this help text
```

One important point to note is that the `parse_args()` call is wrapped in a `try`/`catch` statement. This is because `parse_args()` may throw a runtime exception (e.g. when the user passes an invalid argument) and we want to handle this scenario gracefully. In our example program, we simply print the exception to the console and exit with a non-zero exit code:

```
./hello --verbose
--------------------OUTPUT--------------------
./hello: invalid option 'verbose', pass --help to display possible options
```

The exit code returned by `echo $?` is `1`.

### Positional Arguments

We'll now take a look at parsing positional arguments in our program. We'll change the concept of the original program slightly to print out a name instead of simply 'Hello World!'. We'll also make it so that the amount of exclamation points (!) is configurable. We can add positional parameters for the name and point count like so:

```c++
#include <cparseparse/argument-parser.h>
#include <iomanip>  // For std::setfill and std::setw
#include <iostream>

using namespace cpparse;

int main(int argc, char *argv[]) {
	Argument_Parser parser{};
	parser.set_description("Say hello to someone");
	auto &name_arg = parser.add_positional("name");  // Add positional parameter for 'name'
	auto &points_arg = parser.add_positional("points");  // Add positional parameter for 'points'
	try {
		parser.parse_args(argc, argv);
		const auto name = name_arg.as_type<std::string>();  // Parse 'name' as a string
		const auto points = points_arg.as_type<unsigned int>();  // Parse 'points' as an unsigned int
		
		std::cout << "Hello " << name << std::setfill('!') << std::setw(points) << "" << std::endl;
	} catch (const std::runtime_error &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
```

The `add_positional()` function is used to declare a positional parameter with a given name and return a reference to the corresponding info object. After `parse_args()` is called, we can use each info object's `as_type()` template function to obtain the values of the user-supplied arguments. Similar to `parse_args()`, the `as_type()` function may throw a runtime exception if the parsing fails, so it must be wrapped in a `try`/`catch` statement.

If we compile and run this program same as before with no arguments, we'll now be told to add an additional argument:

```
g++ -Wall -Wextra -std=c++20 -o hello hello.cc
./hello
--------------------OUTPUT--------------------
./hello: requires positional argument 'name'
```

Similarly, only supplying one of the two necessary arguments yields an error:

```
./hello Matt
--------------------OUTPUT--------------------
./hello: requires positional argument 'points'
```

Only when we supply both positional arguments will the program execute:

```
./hello Matt 3
--------------------OUTPUT--------------------
Hello Matt!!!
```

Note that the type passed to the `as_type()` template tells the argument parser how to parse this argument. In the case of `name`, `std::string` is passed which tells the parser to treat the name string as is. For `points`, passing `unsigned int` tells the parser to attempt to read `points` as a unsigned integer. If the user passes a non-integer or a negative integer, the parsing will fail:

```
./hello Matt non-int
# OR
./hello Matt -1
--------------------OUTPUT--------------------
./hello: 'points' must be in range [0,4294967295]
```

The following types can be passed to `as_type()`:
* `bool`
* `char`
* Any signed or unsigned integral type (`short`, `int`, `long`, `std::uint32`, etc.)
* `float`/`double`
* `std::string`

#### Argument Descriptions

By checking the program help with `--help`, we can see that `name` and `points` are now listed as required arguments:

```
Usage: ./hello [options] <name> <points>

  Say hello to someone

Positional arguments:
  name              
  points            

Options:
  -h, --help                  display this help text
```

Unfortunately, this help text tells the user nothing about what `name` and `points` are supposed to be. We can fix this by adding help text for these parameters. The info object returned from `add_positional()` provides the `help()` function. We can modify our `add_positional()` usage like so:

```c++
...
auto &name_arg = parser.add_positional("name").help("name of person to greet");
auto &points_arg = parser.add_positional("points").help("number of exclamation points");
...
```

The `help()` function returns the same info object reference, which allows for convenient function call chaining.

Passing `--help` now gives us this information:

```
Usage: ./hello [options] <name> <points>

  Say hello to someone

Positional arguments:
  name              name of person to greet
  points            number of exclamation points

Options:
  -h, --help                  display this help text
```

### Optional Arguments

In this section, we'll take a look at parsing various types of optional arguments. There are three main forms that optional arguments can take:
* [Single-Value Arguments](#single-value-arguments)
* [Flag Arguments](#flag-arguments)
* [Append-Style Arguments](#append-style-arguments)

We'll begin by looking at single-value arguments.

#### Single Value Arguments

In our sample program,  say that we no longer wish to require that the user provide a number of exclamation points. It might be more convenient to have a default number of points present, and then give the user an option to override this default. This can be accomplished by changing `points` to an optional parameter:

```c++
#include <cparseparse/argument-parser.h>
#include <iomanip>
#include <iostream>

using namespace cpparse;

int main(int argc, char *argv[]) {
	Argument_Parser parser{};
	parser.set_description("Say hello to someone");
	auto &name_arg = parser.add_positional("name").help("name of person to greet");
	// Replace 'add_positional()' with 'add_optional()'
	auto &points_arg = parser.add_optional("-p", "--points").help("number of exclamation points");
	try {
		parser.parse_args(argc, argv);
		const auto name = name_arg.as_type<std::string>();
		const auto points = points_arg.as_type<unsigned int>(1);  // Use '1' as the default
		
		std::cout << "Hello " << name << std::setfill('!') << std::setw(points) << "" << std::endl;
	} catch (const std::runtime_error &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
```

The first difference from before is the syntax of the `add_optional()` call. Whereas `add_positional()` takes a single name option, `add_optional()` takes both a short name and a long name. The short name is a single character preceded by `-` and the long name is the full argument name preceded by `--`. Alternatively, the short name can be omitted to force the user to refer to the argument only by the long name.

The second difference is that the call to `as_type()` now takes the value `1`, which is used as the default number of points when not specified by the user.

We can print the program help text to see the new requirements:

```
./hello --help
--------------------OUTPUT--------------------
Usage: ./hello [options] <name>

  Say hello to someone

Positional arguments:
  name              name of person to greet

Options:
  -h, --help                  display this help text
  -p, --points POINTS         number of exclamation points
```

We can now run the program with only the `name` argument:

```
./hello Matt
--------------------OUTPUT--------------------
Hello Matt!
```

Alternatively, we can use `-p/--points` to change the number of exclamation points. Optional arguments can be specified anywhere in the invocation, either before or after the positional arguments:

```
./hello Matt -p 7
# OR
./hello Matt --points 7
# OR
./hello -p 7 Matt
--------------------OUTPUT--------------------
Hello Matt!!!!!!!
```

#### Flag Arguments

To use other types of the optional arguments, we must pass a special selector value to `add_optional()`. Since single-value arguments are the default type, we were able to omit this selector value before, but this time we'll include it for completeness. Let's add a flag argument called `--show-time` that will print the current time to the user:

```c++
#include <cparseparse/argument-parser.h>
#include <chrono>  // For time functions
#include <iomanip>
#include <iostream>

using namespace cpparse;
using Opt_Type = Optional_Info::Type;  // Alias for Optional_Info::Type
using Clock = std::chrono::system_clock;  // Alias for std::chrono::system_clock

int main(int argc, char *argv[]) {
	Argument_Parser parser{};
	parser.set_description("Say hello to someone");
	auto &name_arg = parser.add_positional("name").help("name of person to greet");
	auto &points_arg = parser.add_optional("-p", "--points", Opt_Type::SINGLE)  // Use type SINGLE
			.help("number of exclamation points");
	auto &show_time_arg = parser.add_optional("--show-time", Opt_Type::FLAG)  // Use type FLAG
			.help("display current system time");
	try {
		parser.parse_args(argc, argv);
		const auto name = name_arg.as_type<std::string>();
		const auto points = points_arg.as_type<unsigned int>(1);
		const auto show_time = show_time_arg.as_type<bool>();  // Parse flag argument
		
		std::cout << "Hello " << name << std::setfill('!') << std::setw(points) << "" << std::endl;
		if (show_time) {
			const auto time = Clock::to_time_t(Clock::now());
			std::cout << "The current time is: " << std::ctime(&time);
		}
	} catch (const std::runtime_error &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
```

Flag arguments are boolean switches that become set when specified by the user. Note that `--show-time` has no default value specified in `as_type()`. This is because flag arguments have an implicit default value of `false`.

By running the program with the `--show-time` argument, we'll see the system time printed:

```
./hello --show-time Matt
--------------------OUTPUT--------------------
Hello Matt!
The current time is: Wed Nov 24 14:02:12 2021
```

#### Append-Style Arguments

The final form for optional arguments is append-style. This form allows the user to specify multiple values for a single argument. In our sample program, let's imagine we want to have it also greet some friends that are with us. We can add an optional append-style parameter `-f/--friend` that will allow us to specify one or more friends:

```c++
#include <cparseparse/argument-parser.h>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace cpparse;
using Opt_Type = Optional_Info::Type;
using Clock = std::chrono::system_clock;

int main(int argc, char *argv[]) {
	Argument_Parser parser{};
	parser.set_description("Say hello to someone");
	auto &name_arg = parser.add_positional("name").help("name of person to greet");
	auto &points_arg = parser.add_optional("-p", "--points", Opt_Type::SINGLE)
			.help("number of exclamation points");
	auto &friends_arg = parser.add_optional("-f", "--friend", Opt_Type::APPEND)  // Use type APPEND
			.help("friend to greet as well");
	auto &show_time_arg = parser.add_optional("--show-time", Opt_Type::FLAG)
			.help("display current system time");
	try {
		parser.parse_args(argc, argv);
		const auto name = name_arg.as_type<std::string>();
		const auto points = points_arg.as_type<unsigned int>(1);
		const auto show_time = show_time_arg.as_type<bool>();
		const auto friends = friends_arg.as_type_all<std::string>();  // Parse all friend values
		
		std::cout << "Hello " << name;
		for (const auto &f : friends)
			std::cout << ", " << f;
		std::cout << std::setfill('!') << std::setw(points) << "" << std::endl;
		if (show_time) {
			const auto time = Clock::to_time_t(Clock::now());
			std::cout << "The current time is: " << std::ctime(&time);
		}
	} catch (const std::runtime_error &ex) {
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
```

The `add_optional()` invocation for `-f/--friend` should look pretty similar to what we've seen before; the only difference being that we use `APPEND` as the type. What's more different is how we go about parsing the new argument. Since the argument can hold multiple values, we need parsing functions capable of returning an entire list of values. The `as_type_all()` function used above does exactly this; all values given for `-f/--friend` are parsed as the specified type (`std::string`) and returned in a `std::vector`. Some other useful functions for dealing with append-style arguments are `count()` and `as_type_at()`, which we could have used in place of `as_type_all()`.

Now, we can run the program again and specify some friends (either with `-f` or `--friend`):

```
./hello Matt -f April --friend Lillian
--------------------OUTPUT--------------------
Hello Matt, April, Lillian!
```

### Overriding Default Help Behavior

By default, the argument parser is initialized with an implicit `-h/--help` flag. When the user passes this flag, the program help text is printed and `std::exit(0)` is called. While this is a common way to handle the help flag, it may be desirable to override this default behavior in some instances.

If we want to modify what happens when the user passes the help flag, we can use the `set_help_handler()` function in `Argument_Parser` to provide our own callback. For example, to display a copyright notice after the usual help text:

```c++
...
Argument_Parser parser{};
parser.set_help_handler([](const Argument_Parser &parser) {
	parser.print_help();
	std::cout << std::endl << "Copyright me 2021" << std::endl;
	std::exit(0);
});
...
```

We can then run the program with the help flag to invoke our custom handler:

```
./hello --help
--------------------OUTPUT--------------------
Usage: ./hello [options] <name>

  Say hello to someone

Positional arguments:
  name              name of person to greet

Options:
  -h, --help                  display this help text
  -p, --points POINTS         number of exclamation points
  -f, --friend FRIEND         friend to greet as well
  --show-time                 display current system time

Copyright me 2021
```

Alternatively, if we wish to disable the implicit `-h/--help` flag altogether, we can set the `auto_help()` option to `false`. This is done by passing a configuration options builder instance to `Argument_Parser` and invoking the `auto_help()` function:

```c++
...
Argument_Parser parser{Argument_Parser::Options{}.auto_help(false)};
...
```

## API Reference

CParseParse is documented via Doxygen and hosted via [GitHub Pages](https://matthewrasa.github.io/cparseparse). Check there for the full API reference.