#include <iostream>
#include <string>
#include <utility>

#include "function_traits.h"

// TODO: Add match overload for string constants (const char [N])
// TODO: Should I add const before the '&' as well ???
// TODO: Allow throwing from the unmatched contract destructor
// TODO: Rework to allow for variant, tuple, and any to be passed

// Interface Classes
class MatchBuilder {};

class Matcher {};

// Helper class to determine if Arg == Function::args that avoids compiler errors
	// Could be done with if constexpr in C++17
template <int arity>
struct MatchResolver {
	template <typename Function, typename Arg>
	constexpr static bool resolve();
};

template <typename T>
struct UnmatchedContract {
	T val;

	UnmatchedContract(T val) : val{ val } {}
	//virtual ~UnmatchedContract() { throw "Non-exhaustive pattern match"; }					// abort is being called
	virtual ~UnmatchedContract() { std::cout << "Non-exhaustive match pattern found"; }

	template <typename F, typename Function = shl::function_traits<F>>
	UnmatchedContract<T>&& operator|(F&& fn);
};

template <typename T, typename F>
class MatchedContract {
	T val;
	F fn;

	MatchedContract(T&& val, F&& fn) : val{ val }, fn{ fn } {}
	~MatchedContract() { fn(val); }

	template <typename FF, typename Tratis = shl::function_traits<FF>>
	MatchedContract<T, F>&& operator|(FF&& fn);
};


// Interface Functions
MatchBuilder&& match();
template <typename T> UnmatchedContract<const T&>&& match(const T& val);

int main() {
	try {
		std::string val = "Hello";

		match(val)
			| [](const std::string& name) { std::cout << "A string\n"; }
			| [](const int& name) { std::cout << "An Int\n"; }
			| []() { std::cout << "Nothing\n"; };
	} catch (std::string& e) {
		std::cout << e;
	}

	std::cin.get();
}


/*
 * Overloads for match function
 */
MatchBuilder&& match() {
	return std::move(MatchBuilder{});
}

template <typename T>
UnmatchedContract<const T&>&& match(const T& val) {
	return std::move(UnmatchedContract<const T&>{ val });
}

UnmatchedContract<const std::string&>&& match(const char* str) {
	return std::move(UnmatchedContract<const std::string&>{ str });
}


/*
 * Overloads for unmatched contracts
 */
template <typename T>
template <typename F, typename Function>
UnmatchedContract<T>&& UnmatchedContract<T>::operator|(F&& fn) {
	bool match = MatchResolver<Function::arity>::resolve<Function, T>();

	std::cout << match << "\n";

	// Resolve function match
	return std::move(*this);
}


/*
 * Overloads for Matched Contracts
 */
template <typename T, typename F>
template <typename FF, typename Traits>
MatchedContract<T, F>&& MatchedContract<T, F>::operator|(FF&& fn) {
	return std::move(*this);
}


/*
 * Overloads for MatchBuilder
 */


/*
 * Overloads for Matcher
 */


/*
 * Overloads for MatchResolver
 */
template<>
template<typename Function, typename Arg>
constexpr bool MatchResolver<0>::resolve() {
	return true;
}

template<>
template<typename Function, typename Arg>
constexpr bool MatchResolver<1>::resolve() {
	return std::is_same<Function::arg<0>::type, Arg>::value;
}

template<int n>
template<typename Function, typename Arg>
constexpr bool MatchResolver<n>::resolve() {
	return false;
}