#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "MatchBuilder.h"
#include "MatchResolver.h"

// TODO: Decide on the performance of the second example (highlights the `const&` issue
	// takes_args apparently needs the `const&` to perform correctly with some conditions
// TODO: Switch over to using perfect-forwarding (`const&` -> `&&`)
// TODO: Figure out how whether MatchBuilder's match will cause compiler issues
// TODO: Get all string cases to work (I think there's a compiler/std bug though)
// TODO: Get match selection to better follow C++ function resolution (ie. when level(a) == level(b))

// TODO: Improve implementation and organization
	// Extract metastructs to a separate file
// TODO: Improve function_traits and has_interface to handle generic lambdas/etc
// TODO: Figure out how to handle non-lambdas
	// Would need to add in extra templates for calling
	// Can't convert from `initializer_list` to `MatchResolver<...>` (How is this even happening?)
// TODO: Add the ability to "return" (get a value) from match
	// Add in `can_produce` struct from the old stash
	// Full support will likely require `std::any`
	// Intermediate support can rely on explicitly specifying the template parameters
		// void would be the default, but i don't know how I would change over
// TODO: Work on actual ADT syntax
// TODO: Allow match selection behavior to be changed from client-code

// TODO: Improve __IndexOf with template<auto> once support is added
// TODO: Replace __IndexOf and __Min with constexpr once support is added (iterating over initalizer_list)
// TODO: Replace num_true with fold expressions once support is added
// TODO: Rework match to work for variant and any once support is added
// TODO: Rework match to work for tuples (I think I need to wait for `std::apply`)
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
	shl::match(c_str)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| [](const char* name) { std::cout << "A cstring\n"; };

	// Should give ???
		// Gives "A string"
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
	  //|| [](const char (&name)[7]) { std::cout << "A literal\n"; };		<- This crashes the compiler because "can't convert from `const char*` to `const char (&)[7]`

	// Should give "Base case"
	shl::match(f)
		| [](const std::string& name) { std::cout << "A string\n"; }
		|| []() { std::cout << "Base case\n"; };

	std::cin.get();
}
