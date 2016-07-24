#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "meta.h"

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
		 * Struct that provides various compile time control structures
		 */
		template<class T>
		struct __Control {
			static constexpr T unless(T a, T b, T c) { return a == b ? c : a; }
			static constexpr T ifneq(T a, T b, T c, T d) { return a != b ? c : d; }
			static constexpr T ifelse(bool s, T a, T b) { return s ? a : b; }
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
	}

	/*
	 * Handles execution of match statement by selecting a function from a list based on argument type matching.
	 *	Matcher doesn't destroy it's function list when matching against a passed value allowing it to be reused
	 *	multiple times if desired without errors.
	 */
	template <class... Fns>
	class Matcher {
		private:
			std::tuple<Fns...> fns;

			template<class T>
			void match_impl(T&& val) {
				using namespace impl;

				// Find the index of the first function that either takes a `T` or is the base case
				constexpr auto min = __Min<takes_args<Fns, T>::level...>::value;								// Find the minimum number of conversions
				constexpr auto index = __Control<size_t>::ifelse(min != -1,										// If a minimum exists
					__IndexOf<size_t, min, 0, takes_args<Fns, T>::level...>::value,								// Take the best match
					__IndexOf<bool, true, 0, base_case<Fns>::value...>::value);									// Otherwise take the base case

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
	template<class T, class... Args>
	void match(T&& val, Matcher<Args...>& matcher) {
		matcher.match(std::forward<T>(val));
	}
}