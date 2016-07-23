#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Decide on the result of the 6th example (this produced "A cstring" before I removed `const&` from the template system)
// TODO: Get all string cases to work (I think there's a compiler/std bug though)
// TODO: Get match selection to better follow C++ function resolution (ie. level(a) == level(b), const T& vs T)
	// I'll probably need to add in a "meta" way of determining better-ness

// TODO: Improve implementation and organization
// TODO: Improve function_traits/et. al. to handle generic lambdas/etc
// TODO: Figure out how to handle non-lambdas
	// Can't convert from `initializer_list` to `MatchResolver<...>` (How is this even happening?)
// TODO: Add the ability to "return" (get a value) from match
// TODO: Actually work on ADT syntax

// TODO: Figure out if it would be possible and beneficial to add match resolution tweaking from client code
	// If I add in the "meta"-better struct, then I can add it as a parameter
// TODO: Improve meta structs with template<auto> once support is added
// TODO: Replace __IndexOf and __Min with constexpr once support is added (iterating over initalizer_list)
// TODO: Replace num_true with fold expressions once support is added
// TODO: Rework match to work for `std::variant` and `std::any` once support is added
// TODO: Find a way to warn about missing '||'			<- Not possible AFAIK
// TODO: Find a way to remove the need for '||' syntax	<- Not possible AFAIK

// TODO: Figure out how to turn this into a benchmarking practice
// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)


int main() {
	auto tup = std::make_tuple(3, "Hello");
	std::string str = "Hello";
	const char* c_str = str.c_str();
	auto f = 3.3f;

	// Should give "Tuple Not Applied"
	shl::match(tup)
		| [](int i, std::string msg) { std::cout << "Tuple Applied"; }
		| [](std::tuple<int, std::string> tup) { std::cout << "Tuple Not Applied\n"; }
		|| []() { std::cout << "Base case\n"; };

	// Should give "Tuple Applied"
	shl::match(tup)
		| [](int i, std::string msg) { std::cout << "Tuple Applied\n"; }
		|| []() { std::cout << "Base case\n"; };

	// Should give "Cstring Tuple"
	shl::match(tup)
		| [](int i, std::string s) { std::cout << "String Tuple\n"; }
		|| [](int i, const char* s) { std::cout << "Cstring Tuple\n"; };

	// Should give "Works properly"
	shl::match(tup)
		| [](std::tuple<int, std::string> tup) { std::cout << "Does not work\n"; }
		|| [](std::tuple<int, const char*> tst) { std::cout << "Works Properly\n"; };

	// Should give "A cstring"
	shl::match(c_str)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| [](const char* name) { std::cout << "A cstring\n"; };

	// Should give "A cstring"
	std::cout << "? ";
	shl::match(c_str)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| [](const char* const& name) { std::cout << "A cstring\n"; };

	// Should give "A string"
	shl::match(c_str)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	// Should give "A string"
	shl::match("World!")
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	// Should give "A literal"
		// Crashes compiler trying to call the second function
	//shl::match("World!")
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| [](const char* name) { std::cout << "A literal\n"; };			// Yet this match isn't selected !
	//	|| [](const char (&name)[7]) { std::cout << "A literal\n"; };		<- This crashes the compiler because "can't convert from `const char*` to `const char (&)[7]`

	// Should give "Base case"
	shl::match(f)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	// Should give ???
	shl::match(3)
		| [](short s) { std::cout << "A short\n"; }
		|| [](long l) { std::cout << "A long\n"; };

	// Should give ???
	shl::match(3)
		| [](long l) { std::cout << "A long\n"; }
		|| [](short s) { std::cout << "A short\n"; };

	std::cin.get();
}
