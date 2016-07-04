#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "has_interface.h"

// TODO: Fix issues with string overloads
// TODO: Implement Matcher::match
// TODO: Rework match to allow for variant and any to be passed
// TODO: Rework match to allow for tuple to be passed
// TODO: Remove explicit void return type in takes_args (needed to allow for chaining)
// TODO: Add 'inline' and 'constexpr' to resolution functions ???
// TODO: Improve exception messages

// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)

// SFINAE Classes
template<class Fn, class... Args>
struct takes_args : shl::has_interface<Fn, void(Args...)> {};

template<class Fn>
struct base_case : shl::has_interface<Fn, void()> {};


// Interface Classes

// class template is necessary as you can't make a function template virtual
template <typename T> struct UnmatchedContract { virtual void eval(T) = 0; };

// class that represents (and holds) a successful match attempt
template<class T, class F>
struct MatchedContract : UnmatchedContract<T>{
	F fn;

	MatchedContract(F&&);
	virtual void eval(T);
};

// class that handles linear resolution of a direct match call
template <class T>
class MatchResolver {
	private:
		T val;
		UnmatchedContract<T>* match;		// Need the polymorphism cause I don't know the type of F just yet

	public:
		MatchResolver(T);
		MatchResolver(MatchResolver&&);
		~MatchResolver();

		template <class F>
		MatchResolver&& operator|(F&& fn);
};

// class that encapsulates a match call for later calls against many values
template<class _Tuple>
class Matcher {
	private:
		_Tuple fns;

		template<class T>
		void match_impl(T);

	public:
		Matcher(_Tuple&&);

		template<class T>
		void operator()(const T&);

		template<class T>
		void match(const T&);
};

// class that constructs a Matcher using similar syntax to a MatchResolver
template <class... Args>
class MatchBuilder {
	private:
		std::tuple<Args...> fns;

	public:
		MatchBuilder(std::tuple<Args...>&&);

		template <class F>
		MatchBuilder<Args..., F> operator|(F&& fn);

		// This syntax is possibly just a temporary measure
			// though `\` doesn't compile for some reason
		template <class F>
		Matcher<std::tuple<Args..., F>> operator||(F&& fn);
};

struct pattern_error : std::runtime_error {
	pattern_error() : std::runtime_error{ "Non-exhaustive pattern match found!" } {};
};

// Helper class to prevent impossible errors from stopping compilation
	// Little cheat to get around the type checker (can replace with 'if constexpr')
template<bool> struct Invoker;


// Interface methods
MatchBuilder<> match();
template<class T> MatchResolver<const T&> match(const T&);



int main() {
	auto test = std::make_tuple();
	std::string val = "Hello";
	const char* str = val.c_str();
	
	// Testing the value matcher
	try {
		match(val)
			| [](const std::string& name) { std::cout << "A string\n"; }
			| [](const int& name) { std::cout << "An int\n"; };
			//| []() { std::cout << "Nothing\n"; };

	} catch (std::string& e) {
		std::cout << e;

	} catch (std::exception& e) {
		std::cout << e.what();
	}

	// Test the matcher object
	auto almost_match = match()
		| [](const std::string& name) { std::cout << "Passed Test\n"; }
		| [](const int& name) { std::cout << "An int\n"; }
		|| [](const std::string& name) { std::cout << "Failed Test\n"; };
		// The `\` gave errors oddly

	almost_match(val);		// Should display Passed Test

	std::cin.get();
}



/*
 * Interface methods
 */
MatchBuilder<> match() {
	return MatchBuilder<>{ std::make_tuple() };
}

template<class T> MatchResolver<const T&> match(const T& val) {
	return MatchResolver<const T&>{ val };
}

// This isn't being called
template <size_t N>
MatchResolver<const std::string&> match(const char(&val)[N]) {
	return MatchResolver<const std::string&>{ val };
}

// Neither is this one
MatchResolver<const std::string&> match(const char* val) {
	return MatchResolver<const std::string&>{ val };
}


/*
 * MatchResolver specializations
 */
template<class T> MatchResolver<T>::MatchResolver(T val) : val{ val }, match{ nullptr } {}
template<class T> MatchResolver<T>::MatchResolver(MatchResolver<T>&& res) : val{ res.val }, match{ res.match } { res.match = nullptr; }
template<class T> MatchResolver<T>::~MatchResolver() {
	// Note: this is why the explicit checks in MatchedContract::eval are necessary to prevent compiler errors (I think)
	if (match) {
		match->eval(val);
		delete match;

	} else {
		delete match;
		throw pattern_error{};									// Why is visual studio calling abort when an exception is thrown ???
	}
}

template<class T> template<class F>
MatchResolver<T>&& MatchResolver<T>::operator|(F&& fn) {
	// if match is nullptr (ie. Unmatched) and the function takes T or the function is the base case, mark fn as the chosen function
	if (!match && (takes_args<F, T>::value || base_case<F>::value))
		match = new MatchedContract<T, F>{ std::move(fn) };

	return std::move(*this);
}


/*
 * Contract eval
 */
template<class T, class F>
MatchedContract<T, F>::MatchedContract(F&& fn) : fn{ fn } {}

template<class T, class F>
void MatchedContract<T, F>::eval(T val) {
	constexpr auto is_base_case = base_case<F>::value;

	if (is_base_case)
		Invoker<is_base_case>::invoke(std::move(fn));

	else
		Invoker<takes_args<F, T>::value>::invoke(std::move(fn), val);		// This will always == !is_base_case. However, I still need to do the `takes_args` to prevent the compiler from crashing
																				// When type checking calls that will never occur (might be able to fix with `if constexpr`)
}


/*
 * Match Builder specializations
 */
template<class... Args> MatchBuilder<Args...>::MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}

// These functions append the new lambda onto the current list (tuple)
template<class ...Args> template<class F>
MatchBuilder<Args..., F> MatchBuilder<Args...>::operator|(F && fn) {
	return MatchBuilder<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
}

// But this one returns the new tuple in a Matcher object instead of a MatchBuilder
template<class ...Args> template<class F>
Matcher<std::tuple<Args..., F>> MatchBuilder<Args...>::operator||(F && fn) {
	return Matcher<std::tuple<Args..., F>>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
}


/*
 * Matcher specializations
 */
template<class _Tuple> Matcher<_Tuple>::Matcher(_Tuple&& fns) : fns{ fns } {}

template<class _Tuple> template<class T>
void Matcher<_Tuple>::match_impl(T val) {
	std::cout << "Matcher has been created\n";
}

template<class _Tuple> template<class T>
void Matcher<_Tuple>::operator()(const T& val) { return match_impl<const T&>(val); }

template<class _Tuple> template<class T>
void Matcher<_Tuple>::match(const T& val) { return match_impl<const T&>(val); }


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

