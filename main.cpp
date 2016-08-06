#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "MatchResolver.h"
#include "Option.h"

// TODO: Rename BetterMatch, et al
	// Change BetterMatchResolver to DefaultMatchResolver
	// Pair down number of function/structs
// TODO: Figure out how to consider tuple cv-ref differences
	// This affects compilation and resolution
// TODO: Clean up implementation
	// Combine takes_args and callable_with
// TODO: Implement ConvRank accurately
// TODO: Implement to handle value cases
// TODO: Look into adding StrictMatchResolver (panics on ambiguous functions)

// TODO: Add in support for default values
// TODO: Fix intellisense

// TODO: Add the ability to "return" (get) a value from match
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
RES_DEF struct BaseCaseSelector {
	static constexpr auto value = NOT_FOUND;
};


template<class F0, class F1, class... Args>
constexpr bool better_match(F0&&, F1&&, Args&&...) {
	return BetterMatch<F0, F1, Args...>::value;
}

int main() {
	auto tupl = std::make_tuple(3, std::string{ "Hello" }.c_str());
	auto str = std::string{ "Hello" };
	auto c_str = str.c_str();

	using shl::impl::BetterResolver;

	// These are the problem cases
	std::cout << "Cstring Tuple     - ";
	shl::match<BetterResolver>(tupl)
		| [](int, std::string) { std::cout << "String Tuple\n"; }
		|| [](int, const char*) { std::cout << "Cstring Tuple\n"; };

	std::cout << "Works properly    - ";
	shl::match<BetterResolver>(tupl)
		| [](std::tuple<int, std::string>) { std::cout << "Does not work\n"; }
		|| [](std::tuple<int, const char*>) { std::cout << "Works Properly\n"; };

	std::cout << "A cstring         - ";
	shl::match<BetterResolver>(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| [](const char*) { std::cout << "A cstring\n"; };

	std::cout << "A cstring         - ";
	shl::match<BetterResolver>(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| [](const char* const&) { std::cout << "A cstring\n"; };

	std::cout << "A string          - ";
	shl::match<BetterResolver>(c_str)
		| [](const std::string&) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "A string          - ";
	shl::match<BetterResolver>("World!")
		| [](const std::string&) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "A literal         - ";
	shl::match<BetterResolver>("World!")
		| [](const char*) { std::cout << "A cstring\n"; }
		|| [](const char(&)[7]) { std::cout << "A literal\n"; };

	std::cout << "A cstring         - ";
	shl::match<BetterResolver>("World!")
		| [](const char*) { std::cout << "A cstring\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "An int            - ";
	shl::match<BetterResolver>(short{ 3 })
		| [](int) { std::cout << "An int\n"; }
		|| [](long) { std::cout << "A long\n"; };

	std::cout << "An int            - ";
	shl::match<BetterResolver>(short{ 3 })
		| [](long) { std::cout << "A long\n"; }
		|| [](int) { std::cout << "An int\n"; };
//*/

	std::cin.get();
}
