/*
#include <iostream>
#include <string>
#include <utility>

#include "has_interface.h"

// TODO: Add match overload for string constants (const char [N])
// TODO: Should I add const before the '&' as well ???
// TODO: Allow throwing from the unmatched contract destructor
// TODO: Rework to allow for variant, tuple, and any to be passed
// TODO: Add 'inline' to resolution functions
	// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)

// Interface Classes
class MatchBuilder {};

class Matcher {};

// Helper class to determine if Arg == Function::args that avoids compiler errors
	// Could be done with if constexpr in C++17
template <size_t arity>
struct _MatchResolver {
	template <typename Function, typename Arg>
	constexpr static bool arg_match();
};

template <typename T>
struct UnmatchedContract {
	virtual void eval(T val);

	constexpr virtual bool isMatched() { return false; }
};

template <typename T, typename F>
class MatchedContract : public UnmatchedContract<T> {
	private:
		F fn;

	public:
		MatchedContract(F&& fn) : fn{ fn } {}

		void eval(T val);
		constexpr virtual bool isMatched() { return true; }
};

template <typename T>
struct MatchResolver {
	private:
		T val;
		UnmatchedContract<T>* con;

	public:
		MatchResolver(T val) : val{ val }, con{ new UnmatchedContract<T>{} } {}
		~MatchResolver();

		template <typename F, typename Traits = shl::function_traits<F>>
		MatchResolver&& operator|(F&& fn);
};


// Interface Functions
MatchBuilder&& match();
template <typename T> MatchResolver<const T&>&& match(const T& val);

int old_main() {
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
	return 0;
}


/*
 * Overloads for match function

MatchBuilder&& match() {
	return std::move(MatchBuilder{});
}

template <typename T>
MatchResolver<const T&>&& match(const T& val) {
	return std::move(MatchResolver<const T&>{ val });
}

MatchResolver<const std::string&>&& match(const char* str) {
	return std::move(MatchResolver<const std::string&>{ str });
}


/*
 * Overloads for MatchResolver

template <typename T>
template <typename F, typename Traits>
MatchResolver<T>&& MatchResolver<T>::operator|(F&& fn) {
	if (!this->con->isMatched() &&_MatchResolver<Traits::arity>::arg_match<Traits, T>()) {
		delete this->con;
		this->con = new MatchedContract<T, F>{ std::move(fn) };
	}

	return std::move(*this);
}

template <typename T>
MatchResolver<T>::~MatchResolver() {
	if (con->isMatched()) {
		con->eval(val);
		delete con;

	} else {
		delete con;
		throw std::string{ "Non-exhaustive pattern match found" };
	}
}


/*
 * Overloads for unmatched contracts

template <typename T>
void UnmatchedContract<T>::eval(T val) {}


/*
 * Overloads for Matched Contracts

// I think this errors because F ~= void(T)
template <typename T, typename F>
void MatchedContract<T, F>::eval(T val) {
	this->fn(val);
}


/*
 * Overloads for MatchBuilder
 */


/*
 * Overloads for Matcher
 */


/*
 * Overloads for MatchResolver

template<>
template<typename Function, typename Arg>
constexpr bool _MatchResolver<0>::arg_match() {
	return true;
}

template<>
template<typename Function, typename Arg>
constexpr bool _MatchResolver<1>::arg_match() {
	return std::is_same<Function::arg<0>::type, Arg>::value;
}

template<size_t n>
template<typename Function, typename Arg>
constexpr bool _MatchResolver<n>::arg_match() {
	return false;
}
//*/