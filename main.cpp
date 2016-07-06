#include <iostream>
#include <string>
#include <utility>
#include <exception>

#include "has_interface.h"

// TODO: Matcher doesn't handle base case
// TODO: Split into multiple files
// TODO: Fix issues with string overloads
// TODO: Rework match to allow for variant and any to be passed
// TODO: Rework match to allow for tuple to be passed
// TODO: Add 'inline' and 'constexpr' to resolution functions ???
// TODO: Remove the 'const &' from the type matching
// TODO: Find a way to warn about missing '||'
// TODO: Improve exception messages

// Turn this into a practice on benchmarking (and explore the improvements of various c++ facilities, ie. && vs const &)

// SFINAE Classes
template<class Fn, class... Args>
struct takes_args : std::is_same<typename shl::function_traits<Fn>::arg_types, std::tuple<Args...>> {};

template<class Fn>
struct base_case : takes_args<Fn> {};


// Interface Classes
// forward declaration for class that handles call-site matching
template <bool, class T, class... Fns>
class MatchResolver;

// class that encapsulates a match call for later calls against many values
template<class... Args>
class Matcher {
	private:
		std::tuple<Args...> fns;

		template<class T>
		void match_impl(T);

	public:
		Matcher(std::tuple<Args...>&&);

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
		Matcher<Args..., F> operator||(F&& fn);
		// This almost fucked me over after I changed Matcher's template from `_Tuple` to `Args...`
			// I'd accidentally forgot to change the return type from `Matcher<std::tuple<Args..., F>>` to `Matcher<Args..., F>` but it still compiled

		template <bool, class T, class... Fns>
		friend class MatchResolver;
};

// An unfinished call-site match
template <bool, class T, class... Fns>
class MatchResolver {
	private:
		T val;
		std::tuple<Fns...> match;

	public:
		MatchResolver(T val) : val{ val }, match{ std::make_tuple() } {}
		MatchResolver(T val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}

		template <class F>
		MatchResolver<false, T, Fns..., F> operator|(F&& fn) {
			return MatchResolver<false, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
		}

		// TODO: I need to find a way to warn about missing these
		template <class F>
		MatchResolver<true, T, Fns..., F> operator||(F&& fn) {
			return MatchResolver<true, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
		}
};

// A finished call-site match
template <class T, class... Fns>
class MatchResolver<true, T, Fns...> {
	private:
		T val;
		Matcher<Fns...> match;

	public:
		MatchResolver(T val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}
		~MatchResolver() { match(val); }
};

// Helper class to prevent impossible errors from stopping compilation
	// Little cheat to get around the type checker (can replace with 'if constexpr')
template<bool> struct Invoker;
template<size_t N, bool... match> struct IndexFinder;

template<size_t N, bool match>
struct IndexFinder<N, match> {
	static constexpr size_t value = match ? N : -1;
};

template <size_t N, bool match, bool... matches>
struct IndexFinder<N, match, matches...> {
	static constexpr size_t value = match ? N : IndexFinder<N + 1, matches...>::value;
};


// Interface methods
MatchBuilder<> match() {
	return MatchBuilder<>{ std::make_tuple() };
}

template<class T> MatchResolver<false, const T&> match(const T& val) {
	return MatchResolver<false, const T&>{val};
}


int main() {
	auto test = std::make_tuple();
	std::string val = "Hello";
	const char* str = val.c_str();
	

	// Testing the value matcher
	try {
		match(val)
			| [](const std::string& name) { std::cout << "A string\n"; }
			| [](const int& name) { std::cout << "An int\n"; }
			| [](const float& c) { std::cout << "Nothing\n"; };

	} catch (std::string& e) {
		std::cout << e;

	} catch (std::exception& e) {
		std::cout << e.what();
	}

	// Test the matcher object
	auto almost_match = match()
		| [](const std::string& name) { std::cout << "Passed Test\n"; }
		| [](const int& name) { std::cout << "An int\n"; }
		|| [](const float& name) { std::cout << "A float\n"; };
		//|| []() { std::cout << "Failed Test\n"; };
		// The `\` gave errors oddly

	almost_match.match(3.3f);

	std::cin.get();
}


/*
 * Match Builder specializations
 */
template<class... Args> MatchBuilder<Args...>::MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}

// These functions append the new lambda onto the current list (tuple)
template<class ...Args> template<class F>
MatchBuilder<Args..., F> MatchBuilder<Args...>::operator|(F&& fn) {
	return MatchBuilder<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
}

// But this one returns the new tuple in a Matcher object instead of a MatchBuilder
template<class ...Args> template<class F>
Matcher<Args..., F> MatchBuilder<Args...>::operator||(F&& fn) {
	return Matcher<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
}


/*
 * Matcher specializations
 */
template<class... Args> Matcher<Args...>::Matcher(std::tuple<Args...>&& fns) : fns{ fns } {}

// Helper for Matcher::match_impl that enables better error messages when a non-exhaustive pattern match is attempted
	// Can clean up definition once `if constexpr` is implemented
template <size_t N, bool valid_index>
struct MatchImplHelper {
	template<class T, class _Tuple>
	static void invoke(_Tuple& fns, T val) {
		std::get<N>(fns)(val);
	}
};

template<size_t N>
struct MatchImplHelper<N, false> {
	template<class T, class _Tuple>
	static void invoke(_Tuple& fns, T val) {
		static_assert(false, "Non-exhaustive pattern match found");
	}
};

template<class... Args> template<class T>
void Matcher<Args...>::match_impl(T val) {
	
	// Find the location of the first function in `fns` that either takes a `T` or is the base case (returns -1 on non-exhaustive pattern)
		// Note: There has to be a better way of doing this
	constexpr size_t index = IndexFinder<0, (takes_args<Args, T>::value || base_case<Args>::value)...>::value;
	
	// Attempt to invoke the correct function (will raise an error if none are selected)
		// The second template argument is for whether the index is valid for the function tuple
		// Note: The `>=` is necessary as the compiler treats a `<` as starting a new template
	MatchImplHelper<index, sizeof...(Args) >= index>::invoke(fns, val);
}

template<class... Args> template<class T>
void Matcher<Args...>::operator()(const T& val) { return match_impl<const T&>(val); }

template<class... Args> template<class T>
void Matcher<Args...>::match(const T& val) { return match_impl<const T&>(val); }


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
