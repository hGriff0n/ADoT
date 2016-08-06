#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "MatchResolver.h"
#include "Option.h"

// TODO: Get callable_with to work correctly
	// I can remove all of the old code once I do this
	// takes_args considers tuple application (i don't think callable with does)
// TODO: Figure out how to consider tuple cv-ref differences
	// This affects compilation and resolution
// TODO: Ensure ConvRank is implemented accurately
	// I don't distinguish within ranks yet

// TODO: Add StrictMatchResolver (panics on ambiguous functions)
// TODO: Add in support for default values
// TODO: Fix intellisense

// TODO: Add the ability to "return" (get) a value from match
	// Add support for value cases (currently ignored)
// TODO: Actually work on ADT syntax

// TODO: Look into improving implementation ala (https://github.com/pfultz2/Fit)
// TODO: Improve meta structs with template<auto> once support is added
// TODO: Replace __IndexOf/count_where_eq with constexpr once support is added (iterating over initalizer_list)
// TODO: Replace num_true with fold expressions once support is added
// TODO: Rework match to work for `std::variant` and `std::any` once support is added
// TODO: Find a way to warn about missing '||' in MatchResolver			<- Not possible AFAIK (must be done as a "standalone")
// TODO: Find a way to remove the need for '||' syntax					<- Not possible AFAIK

// TODO: Figure out how to turn this into a benchmarking practice
// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)

// Selector that always calls the base case
RES_DEF BaseCaseSelector {
	static constexpr auto value = NOT_FOUND;
};

int main() {
	auto tupl = std::make_tuple(3, std::string{ "Hello" }.c_str());
	auto str = std::string{ "Hello" };
	auto c_str = str.c_str();
	auto f = 3.3f;

	// These are the problem cases
	std::cout << "Cstring Tuple     - ";
	shl::match(tupl)
		| [](int, std::string) { std::cout << "String Tuple\n"; }
		| f
		|| [](int, const char*) { std::cout << "Cstring Tuple\n"; };

	std::cout << "Works properly    - ";
	shl::match(tupl)
		| [](std::tuple<int, std::string>) { std::cout << "Does not work\n"; }
		|| [](std::tuple<int, const char*>) { std::cout << "Works Properly\n"; };

	std::cout << "A cstring         - ";
	shl::match(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| [](const char*) { std::cout << "A cstring\n"; };

	std::cout << "A cstring         - ";
	shl::match(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| [](const char* const&) { std::cout << "A cstring\n"; };

	std::cout << "A string          - ";
	shl::match(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "A string          - ";
	shl::match("World!")
		| [](const std::string&) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "A literal         - ";
	shl::match("World!")
		| [](const char*) { std::cout << "A cstring\n"; }
		|| [](const char(&)[7]) { std::cout << "A literal\n"; };

	std::cout << "A cstring         - ";
	shl::match("World!")
		| [](const char*) { std::cout << "A cstring\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "An int            - ";
	shl::match(short{ 3 })
		| [](int) { std::cout << "An int\n"; }
		|| [](long) { std::cout << "A long\n"; };

	std::cout << "An int            - ";
	shl::match(short{ 3 })
		| [](long) { std::cout << "A long\n"; }
		|| [](int) { std::cout << "An int\n"; };
//*/

	std::cin.get();
}
