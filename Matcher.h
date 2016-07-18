#pragma once

#include "function_traits.h"

namespace shl {

	// Helper class to determine the number of true values in a boolean variadic
	// TODO: Replace with fold expressions once support is added
	template <bool b, bool... bools>
	struct num_true {
		static constexpr size_t value = b + num_true<bools...>::value;
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
		static constexpr bool value = sizeof...(Params) == num_true<(std::is_same<Params, Args>::value || std::is_convertible<Args, Params>::value)...>::value;

		// Number of conversions that would be needed to successfully call the function (iff value is true)
		static constexpr size_t level = sizeof...(Params)-num_true<std::is_same<Params, Args>::value...>::value;

		// TODO: Determine whether I need this or not
		template <size_t curr> static constexpr bool better() { return curr > level; }
	};

	template<class... Params, class... Args>
	struct call_matcher_impl<false, std::tuple<Params...>, std::tuple<Args...>> {
		static constexpr bool value = false;
		static constexpr size_t level = -1;
		template <size_t curr> static constexpr bool better() { return false; }
	};

	// SFINAE traits class to determine matching that accounts for implicit conversions
	template<class... Params, class... Args>
	struct call_matcher<std::tuple<Params...>, std::tuple<Args...>> : call_matcher_impl<sizeof...(Params) == sizeof...(Args), std::tuple<Params...>, std::tuple<Args...>> {};


	// SFINAE traits classes (for matching functions based on arguments)
	template<class Fn, class... Args>
	struct takes_args : call_matcher<typename shl::function_traits<Fn>::arg_types, std::tuple<Args...>> {};

	// Class for matching against the base case
	template<class Fn>
	struct base_case : std::is_same<typename shl::function_traits<Fn>::arg_types, std::tuple<>> {};


	namespace impl {

		/*
		 * Given a boolean sequence, find the location of the first `true`
		 *	Is there a way to do this with constexpr ???
		 */
		template <size_t N, bool match, bool... matches>
		struct __IndexOf {
			static constexpr size_t value = match ? N : __IndexOf<N + 1, matches...>::value;
		};

		// The list has been exhuasted
		template<size_t N, bool match>
		struct __IndexOf<N, match> {
			static constexpr size_t value = match ? N : -1;
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
	}

	/*
	 * Handles execution of match statement by selecting a function from a list based on argument type matching.
	 *	Matcher doesn't destroy it's function list when matching against a passed value allowing it to be reused
	 *	multiple times if desired without errors.
	 */
	template <class... Args>
	class Matcher {
		private:
			std::tuple<Args...> fns;

			template<class T>
			void match_impl(T val) {
				using namespace impl;

				// Find the index of the first function that either takes a `T` or is the base case
					// Note: There has to be a better way of doing this
				constexpr size_t index = __IndexOf<0, (takes_args<Args, T>::value || base_case<Args>::value)...>::value;

				// Raise a compiler error if no function was found
				static_assert(index < sizeof...(Args), "Non-exhaustive pattern match found");

				// Call the choosen function
				__MatchHelper::nice_invoke<index>(fns, val);
				//__MatchHelper::invoke(std::get<index>(fns), val);				// std::get raises compiler errors in spite of the static_assert
			}

		public:
			Matcher(std::tuple<Args...>&& fns) : fns{ fns } {}

			template<class T> void operator()(const T& val) { return match_impl<const T&>(val); }

			template<class T> void match(const T& val) { return match_impl<const T&>(val); }
	};

	// Pass the value on to the provided matcher object for match resolution
	template<class T, class... Args>
	void match(const T& val, Matcher<Args...>& matcher) {
		matcher.match(val);
	}
}