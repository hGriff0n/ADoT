#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Fix issues with string overloads
// TODO: Rework match to allow for variant and any to be passed
// TODO: Rework match to allow for tuple to be passed
// TODO: Add 'inline' and 'constexpr' to resolution functions ???
// TODO: Remove the 'const &' from the type matching
// TODO: Find a way to warn about missing '||'

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
