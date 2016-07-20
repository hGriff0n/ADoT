#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Find a better way of resolving multiple selections (current way doesn't ape C++ function resolution)
// TODO: Allow convertible selection to be changed in client-code
// TODO: Improve utility of takes_args and base_case (add callable filtering, etc.)
	// There's a problem with constructing the MatchResolver for testing (no constructor found)
// TODO: Remove the 'const &' from the type system
// TODO: Get all string cases to work (I think there's a compiler/std bug though)

// TODO: Improve implementation and organization
// TODO: Improve __IndexOf with template<auto> once support is added
// TODO: Replace num_true with fold expressions once support is added
// TODO: Rework match to work for variant and any once support is added
// TODO: Rework match to work for tuples (I think I need to wait for `std::apply`)
// TODO: Improve function_traits and has_interface to handle generic lambdas/etc
// TODO: Figure out how to handle non-lambdas
// TODO: Work on actual ADT syntax
// TODO: Find a way to warn about missing '||'			<- Not possible AFAIK
// TODO: Find a way to remove the need for '||' syntax	<- Not possible AFAIK

// TODO: Figure out how to turn this into a benchmarking practice
// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)


int main() {
	auto tup = std::make_tuple();
	std::string str = "Hello";
	const char* c_str = str.c_str();
	auto f = 3.3f;

	// Should give "A cstring"
	//shl::match(c_str)
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| [](const char* const& name) { std::cout << "A cstring\n"; };

	// Should give "A string"
	//shl::match(c_str)
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| []() { std::cout << "Base case\n"; };

	// Should give "A string"
	//shl::match("World!")
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| []() { std::cout << "Base case\n"; };

	// Should give "A literal"
		// Crashes compiler trying to call the second function
	//shl::match("World!")
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| [](const char* name) { std::cout << "A literal\n"; };			// Yet this match isn't selected !
	  //|| [](const char (&name)[7]) { std::cout << "A literal\n"; };		<- This crashes the compiler because "can't convert from `const char*` to `const char (&)[7]`

	// Should give "Base case"
	//shl::match(f)
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| []() { std::cout << "Base case\n"; };

	auto l = []() {};
	std::cout << shl::is_callable<decltype(l)>::value;

	std::cin.get();
}
