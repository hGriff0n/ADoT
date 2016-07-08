#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Fix issues with string overloads
// TODO: Find a way to warn about missing '||'
// TODO: Remove the 'const &' from the type system
// TODO: Rework match to allow for variant and any to be passed <- Need to wait for C++17 support
// TODO: Rework match to allow for tuple to be passed			<- Might need to wait for C++17 (for `std::apply` I think)
// TODO: MSVC supports fold expressions, can I use this in some way ???

// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)


int main() {
	auto tup = std::make_tuple();
	std::string str = "Hello";
	const char* c_str = str.c_str();
	auto f = 3.3f;
	

	// Testing the value matcher
	shl::match(f)
		| [](const std::string& name) { std::cout << "A string\n"; }
		| [](const int& name) { std::cout << "An int\n"; }
		//| [](const float& c) { std::cout << "Nothing\n"; }
		|| []() { std::cout << "Base case\n"; };
		;

	// Test the matcher object
	auto almost_match = shl::match()
		| [](const std::string& name) { std::cout << "Passed Test\n"; }
		| [](const int& name) { std::cout << "An int\n"; }
		|| [](const float& name) { std::cout << "A float\n"; };
		//|| []() { std::cout << "Failed Test\n"; };
		// The `\` gave errors oddly

	almost_match.match(f);

	shl::match(str, almost_match);

	std::cin.get();
}
