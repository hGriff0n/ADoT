#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Allow convertible cases to be selected
	// Implement "Better Match" resolution
		// Implement __UnlessOr and __Min
	// Find a better way of resolving multiple selections
// TODO: Allow convertible selection to be changed in client-code
// TODO: Improve utility of takes_args and base_case (add callable blocking, etc.)
// TODO: Remove the 'const &' from the type system

// TODO: Replace num_true with fold expressions once support is added
// TODO: Improve function_traits and has_interface to handle generic lambdas/etc
	// Note: I don't need has_interface in it's current iteration
// TODO: Figure out how to handle non-lambdas
// TODO: Work on actual ADT syntax
// TODO: Find a way to warn about missing '||'			<- Not possible AFAIK
// TODO: Find a way to remove the need for '||' syntax	<- Not possible AFAIK
// TODO: Rework match to allow for variant and any to be passed <- Need to wait for C++17 support
// TODO: Rework match to allow for tuple to be passed			<- Might need to wait for C++17 (for `std::apply` I think)

// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)


int main() {
	auto tup = std::make_tuple();
	std::string str = "Hello";
	const char* c_str = str.c_str();
	auto f = 3.3f;

	// Should give "A cstring"
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
		// Gives "A string"
	//shl::match("World!")
	//	| [](const std::string& name) { std::cout << "A string\n"; }
	//	|| [](const char(&name)[7]) { std::cout << "A literal\n"; };

	// Should give "Base case"
	shl::match(f)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cin.get();
}
