#pragma once

#include "has_interface.h"

namespace shl {
	template<class, class> struct call_matcher;

	namespace impl {
		// Counts the number of ocurrances of eq in the parameter pack
		template<class T, T eq, T b, T... ts>
		struct count_where_eq {
			static constexpr size_t value = (eq == b) + count_where_eq<ts...>::value;
		};

		template<class T, T eq, T b>
		struct count_where_eq<T, eq, b> {
			static constexpr size_t value = (eq == b);
		};

		// Specialization for booleans to accomodate existing code
		template <bool... bs>
		struct num_true : count_where_eq<bool, true, bs...> {};

		// Impl class to enable safe handling of argument and parameter lists that don't match in arity
		template<bool, class, class> struct __CallMatcher;

		template<class... Params, class... Args>
		struct __CallMatcher<true, std::tuple<Params...>, std::tuple<Args...>> {
			// Can the function be called with the arguments (ie. do the arg types match or can be converted to the function types)
			// Note: The ordering of Param and Arg is important in std::is_convertible<From, To>
			static constexpr bool value = sizeof...(Params) == num_true<std::is_convertible<Args, Params>::value...>::value;

			// Number of conversions that would be needed to successfully call the function (min is the best match)
			static constexpr size_t level = value ? sizeof...(Params) - num_true<std::is_same<Params, Args>::value...>::value : -1;
		};

		template<class... Params, class... Args>
		struct __CallMatcher<false, std::tuple<Params...>, std::tuple<Args...>> {
			static constexpr bool value = false;
			static constexpr size_t level = -1;		// A level of 0 indicates a perfect match
		};


		// Impl class to enable safe handling of types that aren't callable
		template<bool, class Fn, class... Args>
		struct __TaksArgs : call_matcher<typename function_traits<Fn>::arg_types, std::tuple<Args...>> {};
		template<class Fn, class... Args>
		struct __TaksArgs<false, Fn, Args...> : __CallMatcher<false, Fn, Args...> {};


		// Impl class to enable safe handling of types that aren't callable
		template<bool, class Fn>
		struct __BaseCase {
			static constexpr bool value = shl::function_traits<Fn>::arity == 0;
		};
		template<class Fn> struct __BaseCase<false, Fn> : std::false_type {};


		// Impl class to enable handling of non-function types 
		template<bool, class Fn, class Ret>
		struct __CanProduce : std::is_convertible<typename shl::function_traits<Fn>::ret_type, Ret> {};

		template<class Arg, class Ret>
		struct __CanProduce<false, Arg, Ret> : std::is_convertible<Arg, Ret> {};
	}

	// SFINAE traits class to determine matching that accounts for implicit conversions
	template<class... Params, class... Args>
	struct call_matcher<std::tuple<Params...>, std::tuple<Args...>> : impl::__CallMatcher<sizeof...(Params) == sizeof...(Args), std::tuple<Params...>, std::tuple<Args...>> {};


	// SFINAE wrapper that extracts the function arg types for call_matcher
	template<class Fn, class... Args>
	struct takes_args : impl::__TaksArgs<is_callable<Fn>::value, Fn, Args...> {};


	// Simple wrapper that recognizes the base case function
	template<class Fn>
	struct base_case : impl::__BaseCase<is_callable<Fn>::value, Fn> {};


	// Simple wrapper to test whether the object can produce a given type 
	template<class Fn, class Ret>
	struct can_produce : impl::__CanProduce<is_callable<Fn>::value, Fn, Ret> {};
}