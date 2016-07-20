#pragma once

//#include "function_traits.h"
#include "has_interface.h"

namespace shl {

	// Helper class to determine the number of true values in a boolean variadic
	template <bool b, bool... bs>
	struct num_true {
		static constexpr size_t value = b + num_true<bs...>::value;
	};

	template<bool b> struct num_true<b> {
		static constexpr size_t value = b;
	};

	// Impl class to enable safe handling of argument and parameter lists that don't match in arity
	template<bool, class, class> struct call_matcher_impl;
	template<class, class> struct call_matcher;

	template<class... Params, class... Args>
	struct call_matcher_impl<true, std::tuple<Params...>, std::tuple<Args...>> {
		// Can the function be called with the arguments (ie. do the arg types match or can be converted to the function types)
			// Note: The ordering of Param and Arg is important in std::is_convertible<From, To>
		static constexpr bool value = sizeof...(Params) == num_true<std::is_convertible<Args, Params>::value...>::value;

		// Number of conversions that would be needed to successfully call the function (min is the best match)
		static constexpr size_t level = value ? sizeof...(Params) - num_true<std::is_same<Params, Args>::value...>::value : -1;
	};

	template<class... Params, class... Args>
	struct call_matcher_impl<false, std::tuple<Params...>, std::tuple<Args...>> {
		static constexpr bool value = false;
		static constexpr size_t level = -1;		// A level of 0 indicates a perfect match
	};

	// SFINAE traits class to determine matching that accounts for implicit conversions
	template<class... Params, class... Args>
	struct call_matcher<std::tuple<Params...>, std::tuple<Args...>> : call_matcher_impl<sizeof...(Params) == sizeof...(Args), std::tuple<Params...>, std::tuple<Args...>> {};


	// SFINAE wrapper that extracts the function arg types for call_matcher
	template<class Fn, class... Args>
	struct takes_args : call_matcher<std::enable_if_t<shl::is_callable<Fn>::value, typename shl::function_traits<Fn>::arg_types>, std::tuple<Args...>> {};

	// Simple wrapper that recognizes the base case function
	template<bool, class Fn>
	struct base_case_impl {
		static constexpr bool value = shl::function_traits<Fn>::arity == 0;
	};

	template<class Fn>
	struct base_case_impl<false, Fn> : std::false_type {};

	// Adding callable protection to base_case caused the problem
	template<class Fn>
	struct base_case : base_case_impl<is_callable<Fn>::value, Fn> {};

	// Add in callable support to base_case and takes_args


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
			// Handle dispatching to function if the function takes 0 or 1+ arguments
			template <class F, class T>
			static std::enable_if_t<base_case<F>::value> invoke(F&& fn, T val) {
				fn();
			}

			template <class F, class T>
			static std::enable_if_t<!base_case<F>::value> invoke(F&& fn, T val) {
				fn(val);
			}

			// I can remove this and the size_t template (see commented code in Matcher), but this makes nicer compiler errors
			template <size_t N, class T, class... Args>
			static void nice_invoke(std::tuple<Args...>& fns, T val) {
				__MatchHelper::invoke(std::get<N>(fns), val);
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
			void match_impl(T val) {
				using namespace impl;

				// Find the index of the first function that either takes a `T` or is the base case
				constexpr auto min = __Min<takes_args<Fns, T>::level...>::value;					// Find the minimum number of conversions
				constexpr auto index = __Control<size_t>::ifelse(min != -1,
					__IndexOf<size_t, min, 0, takes_args<Fns, T>::level...>::value,					// Take the best match if one exists
					__IndexOf<bool, true, 0, base_case<Fns>::value...>::value);						// Otherwise take the base case

				static_assert(is_callable<decltype(std::get<0>(fns))>::value, "call<0>");				// get<0> isn't callable
				static_assert(is_callable<decltype(std::get<1>(fns))>::value, "call<1>");				// Neither is get<1>			<- How ???
																												// And how is index != -1 ???

				static_assert(!base_case<decltype(std::get<0>(fns))>::value, "get<0>");					// get<0> is not a base_case
				static_assert(base_case<decltype(std::get<1>(fns))>::value, "get<1>");					// get<1> is a base_case (This is false)

				// Raise a compiler error if no function was found
				static_assert(index < sizeof...(Fns), "Non-exhaustive pattern match found");

				// Call the choosen function
				__MatchHelper::nice_invoke<index>(fns, val);
				//__MatchHelper::invoke(std::get<index>(fns), val);				// std::get raises compiler errors in spite of the static_assert
			}

		public:
			Matcher(std::tuple<Fns...>&& fns) : fns{ fns } {}

			template<class T> void operator()(const T& val) { return match_impl<const T&>(val); }

			template<class T> void match(const T& val) { return match_impl<const T&>(val); }
	};

	// Pass the value on to the provided matcher object for match resolution
	template<class T, class... Args>
	void match(const T& val, Matcher<Args...>& matcher) {
		matcher.match(val);
	}
}