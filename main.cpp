#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchResolver.h"
#include "Option.h"
#include "Conversion.h"

// TODO: Fully test BetterMatch
// TODO: Merge `Conversion.h`, `Matcher.h`, and `function_traits.h`
// TODO: Rename BetterMatch, et al
	// Change BetterMatchResolver to DefaultMatchResolver
// TODO: Clean up implementation
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
	static constexpr auto value = shl::impl::__IndexOf<bool, true, 0, shl::base_case<Fns>::value...>::value;
};

struct Test {
	operator int() { return 3; }
};

template<class W, class Ts>
struct All : std::is_base_of<W, Ts> {};

// TODO: Improve ConvRank coverage
// TODO: Fully Test BetterMatch
int main() {
	auto tup = std::make_tuple(3, "Hello");
	std::string str = "Hello";
	const char* c_str = str.c_str();
	auto f = 3.3f;

	std::cout << "Short(1) - " << ConvRank<short, int>::value
		<< "\nInt(0) - " << ConvRank<int, int>::value
		<< "\nLong(2) - " << ConvRank<long, int>::value
		<< "\nFloat(2) - " << ConvRank<float, int>::value
		<< "\nTuple<int>(2) - " << ConvRank<std::tuple<int>, std::tuple<double>>::value
		<< "\nTest(3) - " << ConvRank<Test, int>::value
		<< "\nString(" << (size_t)-1 << ") - " << ConvRank<std::string, int>::value << "\n";
	
	auto tf_fns = std::make_tuple(
		[](int i, std::string) {},
		[](std::tuple<int, std::string>) {},
		[](std::string) {},
		[](const char*) {},
		[](const std::string&) {},
		[](int) {},
		[](long) {},
		[](short) {},
		[](const char(&)[7]) {}
	);

	auto fn_strs = std::make_tuple(
		"f(int, string)",
		"f(tuple<int, string>)",
		"f(string)",
		"f(const char*)",
		"f(int)",
		"f(long)",
		"f(short)",
		"f(const char(&)[7])"
	);

	using std::get;
	std::cout << "[0] " << get<0>(fn_strs) << " - " << get<1>(fn_strs) << " = " << BetterMatch<decltype(get<0>(tf_fns)), decltype(get<1>(tf_fns)), int, std::string>::value << "\n"
		      << "[1] " << get<0>(fn_strs) << " - " << get<1>(fn_strs) << " = " << BetterMatch<decltype(get<0>(tf_fns)), decltype(get<1>(tf_fns)), std::tuple<int, std::string>>::value << "\n";


	//auto tf = [](int i) { std::cout << i << " is an int\n"; };
	//std::declval<decltype(tf)>()(3);

	shl::match<BetterResolver>(short{ 3 })
		| [](int) { std::cout << "An int\n"; }
		|| [](long) { std::cout << "A long\n"; };

	shl::match<BetterResolver>(short{ 3 })
		| [](long) { std::cout << "A long\n"; }
		|| [](int) { std::cout << "An int\n"; };

	/*/
	std::cout << "Expected          - Received\nTuple Not Applied - ";
	shl::match(tup)
		| [](int i, std::string msg) { std::cout << "Tuple Applied"; }
		| [](std::tuple<int, std::string> tup) { std::cout << "Tuple Not Applied\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "Tuple Applied     - ";
	shl::match(tup)
		| [](int i, std::string msg) { std::cout << "Tuple Applied\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cout << "A string          - ";
	shl::match("World!")
		| [](std::string n) { std::cout << "A string\n"; }
		|| [](const char* l) { std::cout << "A cstring\n"; };				// Note: `string` and `const char*` have equal fit to a string literal

	std::cout << "Base case         - ";
	shl::match(f)
		| [](const std::string& name) { std::cout << "A string\n"; }
		| f																	// This case is never considered currently (worst fit, See "meta.h:141")
		|| []() { std::cout << "Base case\n"; };

	std::cout << "An int            - ";
	shl::match(short{3})
		| [](int s) { std::cout << "A short\n"; }
		|| [](long l) { std::cout << "An int\n"; };

	std::cout << "An int            - ";
	shl::match(short{3})
		| [](long l) { std::cout << "A long\n"; }
		|| [](int s) { std::cout << "An int\n"; };
	//*/

	std::cin.get();
}
