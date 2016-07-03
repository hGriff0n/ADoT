#include <iostream>
#include <string>
#include <utility>

#include "has_interface.h"

// TODO: Add match overload for string constants (const char [N])
// TODO: Add Non-exhaustive pattern exception class
// TODO: Add MatchBuilder and Matcher classes
// TODO: Rework match to allow for variant and any to be passed
// TODO: Rework match to allow for tuple to be passed
// TODO: Remove explicit void return type in takes_args (needed to allow for chaining)
// TODO: Add 'inline' to resolution functions ???

// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)

// Interface Classes
template<class Fn, class... Args>
struct takes_args : shl::has_interface<Fn, void(Args...)> {};

template<class Fn>
struct base_case : shl::has_interface<Fn, void()> {};

template<class T>
struct UnmatchedContract {
	virtual void eval(T val);
	virtual constexpr operator bool() { return false; }
};

template<class T, class F>
struct MatchedContract : UnmatchedContract<T>{
	F fn;
	MatchedContract(F&&);

	virtual void eval(T val);
	constexpr operator bool() { return true; }
};

template <class T>
class MatchResolver {
	private:
		T val;
		UnmatchedContract<T>* match;

	public:
		MatchResolver(T);
		MatchResolver(MatchResolver&&);
		~MatchResolver();

		template <class F>
		MatchResolver&& operator|(F&& fn);
};

// Helper class to prevent impossible errors from stopping compilation
	// Little cheat to get around the type checker (can replace with 'if constexpr')
template<bool> struct Invoker;


// Interface methods
template<class T> MatchResolver<const T&> match(const T& val);


int main() {
	auto test = std::make_tuple(3);
	
	try {
		match(test)
			| [](const std::string& name) { std::cout << "A string\n"; }
			| [](const int& name) { std::cout << "An int\n"; }
			| []() { std::cout << "Nothing\n"; };

	} catch (std::string& e) {
		std::cout << e;
	}

	std::cin.get();
}



/*
 * Interface methods
 */
//Matcher match() { return Matcher{}; }

template<class T> MatchResolver<const T&> match(const T& val) {
	return MatchResolver<const T&>{ val };
}


/*
 * MatchResolver creation/destruction
 */
template<class T> MatchResolver<T>::MatchResolver(T val) : val{ val }, match{ new UnmatchedContract<T>{} } {}
template<class T> MatchResolver<T>::MatchResolver(MatchResolver<T>&& res) : val{ res.val }, match{ res.match } { res.match = nullptr; }
template<class T> MatchResolver<T>::~MatchResolver() {
	if (*match) {
		std::cout << "Calling\n";
		match->eval(val);
		delete match;
	} else {
		delete match;
		throw std::string{ "Non-exhaustive pattern found" };	// Why is visual studio calling abort when an exception is thrown ???
	}
}


/*
 * operator| magic overloads
 */
template<class T> template<class F>
MatchResolver<T>&& MatchResolver<T>::operator|(F&& fn) {
	constexpr auto fn_matches = takes_args<F, T>::value;
	constexpr auto is_base_case = base_case<F>::value;

	std::cout << fn_matches << " - " << is_base_case << "\n";

	if (!*match && (fn_matches || is_base_case)) {
		//Invoker<matches>::invoke(std::move(fn), val);			// Can't replace matches with true as that would cause the compiler to try to type check invalid functions (compilation error)

		delete match;
		match = new MatchedContract<T, F>{ std::move(fn) };		// type cast from MatchedContract<F>* to UnmatchedContract* exists but is inaccessible, How?
	}

	return std::move(*this);
}


/*
 * Contract eval
 */
template<class T, class F>
MatchedContract<T, F>::MatchedContract(F&& fn) : fn{ fn } {}
template<class T> void UnmatchedContract<T>::eval(T val) {}

// This solution doesn't work for the base case
template<class T, class F>
void MatchedContract<T, F>::eval(T val) {
	constexpr auto is_base_case = base_case<F>::value;

	if (is_base_case)
		Invoker<is_base_case>::invoke(std::move(fn));

	else
		Invoker<takes_args<F, T>::value>::invoke(std::move(fn), val);
}


/*
 * Invoker specializations
 */
template<> struct Invoker<true> {
	template <class F>
	static void invoke(F&& fn) {
		fn();
	}

	template <class F, class T>
	static void invoke(F&& fn, T val) {
		fn(val);
	}
};

template<> struct Invoker<false> {
	template <class F>
	static void invoke(F&& fn) {}
	template <class F, class T>
	static void invoke(F&& fn, T val) {}
};