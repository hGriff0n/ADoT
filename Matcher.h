#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "meta.h"

#define RES_CLASS template<class, class...> class												// Template definition to accept a resolution meta-struct as a template parameter
#define RES_DEF template<class Arg, class... Fns>												// Template definition to declare a resolution meta-struct

namespace shl {
	namespace impl {

		/*
		 * Given a pack sequence, find the location of the first `val`
		 *	Is there a way to do this with constexpr ???
		 */
		template <class T, T val, size_t N, T match, T... matches>
		struct __IndexOf {
			static constexpr size_t value = (match == val) ? N : __IndexOf<T, val, N + 1, matches...>::value;
		};

		// The list has been exhuasted
		template<class T, T val, size_t N, T match >
		struct __IndexOf<T, val, N, match> {
			static constexpr size_t value = (match == val) ? N : -1;
		};

		/*
		 * Helper struct for Matcher that enables compile-time checking of pattern exhaustiveness and
		 *	function dispatching. Uses `std::enable_if` to create two mutually exclusive functions
		 *	for handling each case separately without resolution space corruption.
		 *
		 *	`invoke` - Handle dispatch to the base case and normal match statements
		 *	`nice_invoke` - Gives nicer compiler errors (prevents std::get<N> from producing any) if a non-exhaustive pattern is found
		 *
		 *	Can clean up SFINAE functions when `if constexpr` is implemented
		 */
		struct __MatchHelper {
			// Dispatch to the base case (callable<F> == true if base_case<F> == true)
			template <class F, class T>
			static std::enable_if_t<base_case<F>::value> invoke(F&& fn, T&& val) {
				fn();
			}

			// Dispatch to a function that accepts arguments
			template <class F, class T>
			static std::enable_if_t<!base_case<F>::value && callable<F>::value> invoke(F&& fn, T&& val) {
				fn(std::forward<T>(val));
			}

			// Apply tuple to the chosen function (only created if the function takes the decomposed tuple)
			template <class F, class... T>
			static std::enable_if_t<!base_case<F>::value && takes_args<F, T...>::value> invoke(F&& fn, std::tuple<T...> val) {
				std::apply(std::forward<F>(fn), std::forward<std::tuple<T...>>(val));
			}

			// Dispatch to a non-function value
			template <class F, class T>
			static std::enable_if_t<!callable<F>::value> invoke(F&& fn, T&& val) {
				fn;
			}

			// I can remove this and the size_t template (see commented code in Matcher), but this makes nicer compiler errors
			template <size_t N, class T, class... Args>
			static void nice_invoke(std::tuple<Args...>& fns, T&& val) {
				invoke(std::get<N>(fns), std::forward<T>(val));
			}

		};


		/*
		 * Struct to find the minimum of a template pack at compile time
		 */
		template<size_t a, size_t b, size_t... ts> struct __Min {
			static constexpr size_t value = a < b ? __Min<a, ts...>::value : __Min<b, ts...>::value;
		};

		template<size_t a, size_t b> struct __Min<a, b> {
			static constexpr size_t value = a < b ? a : b;
		};


		/*
		 * Default struct to determine the best function to match the args
		 *	Mimics C++ function resolution as much as possible
		 */
		RES_DEF struct __CppResolver {
			private:
				static constexpr auto min = impl::__Min<takes_args<Fns, Arg>::level...>::value;

			public:
				static constexpr auto value = min != -1 ? impl::__IndexOf<size_t, min, 0, takes_args<Fns, Arg>::level...>::value : -1;		// min == -1 iff takes_args<Fns, T> == -`
		};
	}

	/*
	 * Handles execution of match statement by selecting a function from a list based on argument type matching.
	 *	Matcher doesn't destroy it's function list when matching against a passed value allowing it to be reused
	 *	multiple times if desired without errors.
	 */
	template<RES_CLASS Best, class... Fns>
	class Matcher {
		private:
			std::tuple<Fns...> fns;

			template<class T>
			void match_impl(T&& val) {
				using namespace impl;

				// Find the index of the first function that either takes a `T` or is the base case
				constexpr auto best = Best<T, Fns...>::value;
				constexpr auto index = best != -1 ? best : __IndexOf<bool, true, 0, base_case<Fns>::value...>::value;

				// Raise a compiler error if no function was found (Note: don't create a match with > 4 million cases)
				static_assert(index < sizeof...(Fns), "Non-exhaustive pattern match found");

				// Call the choosen function
				__MatchHelper::nice_invoke<index>(fns, std::forward<T>(val));									// Hide compiler errors from `std::get` when index >= sizeof...(Fns)
			}

		public:
			Matcher(std::tuple<Fns...>&& fns) : fns{ fns } {}

			template<class T> void operator()(T&& val) { return match_impl(std::forward<T>(val)); }
			template<class T> void match(T&& val) { return match_impl(std::forward<T>(val)); }
	};

	// Pass the value on to the provided matcher object for match resolution
	template<RES_CLASS Best, class T, class... Args>
	void match(T&& val, Matcher<Best, Args...>& matcher) {
		matcher.match(std::forward<T>(val));
	}
}