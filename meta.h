#pragma once

#include "function_traits.h"

// TODO: Remove when std::apply is implemented
namespace std {
	template<class F, class T, std::size_t... I>
	constexpr auto apply_impl(F&& f, T&& t, std::index_sequence<I...>) {
		return std::invoke(std::forward<F>(f), std::get<I>(std::forward<T>(t))...);
	}

	template<class F, class T>
	constexpr auto apply(F&& f, T&& t) {
		return apply_impl(std::forward<F>(f), std::forward<T>(t), std::make_index_sequence<std::tuple_size<std::decay_t<T>>::value>{});
	}

	/*
	template<class F, class V, class... Param>
	constexpr auto apply_impl(F&& f, V&& v) {
		return std::invoke(std::forward<F>(f), std::get<Param>(std::forward<V>(v))...);
	}

	// Only problem is in how to get the templates
	template<class F, class... Args>
	constexpr auto apply_impl<typename shl::function_traits<F>::arg_types>(F&& f, std::variant<Args...>&& v) {
		
	}
	*/
}

namespace shl {
	template<class, class> struct call_matcher;

	namespace impl {
		// Preserve typing for string literals (std::decay strips away the size, etc.
		template<class Arg>
		struct decay : std::decay<Arg> {};

		template<class C, size_t N>
		struct decay<C(&)[N]> {
			using type = C(&)[N];
		};

		template<class T>
		using decay_t = typename decay<T>::type;


		// Counts the number of ocurrances of eq in the parameter pack
		//template<auto eq, auto b, auto... ts>
		template<class T, T eq, T b, T... ts>
		struct count_where_eq {
			static constexpr size_t value = (eq == b) + count_where_eq<T, eq, ts...>::value;
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
				// Note: The ordering of Param and Arg is important in std::is_convertible
			static constexpr bool value = sizeof...(Params) == num_true<std::is_convertible<Args, Params>::value...>::value;

			// Number of conversions that would be needed to successfully call the function (min is the best match)
				// Note|TODO: This might be changed once I determine how to better implement resolution
			static constexpr size_t level = value ? sizeof...(Params) - num_true<std::is_same<impl::decay_t<Params>, Args>::value...>::value : -1;
		};

		template<class... Params, class... Args>
		struct __CallMatcher<false, std::tuple<Params...>, std::tuple<Args...>> {
			static constexpr bool value = false;
			static constexpr size_t level = -1;				// A level of 0 indicates a perfect match
		};


		// Impl class to enable safe handling of types that aren't callable
		template<bool, class Fn, class... Args>
		struct __TakesArgs : call_matcher<typename function_traits<Fn>::arg_types, std::tuple<Args...>> {};
		template<class Fn, class... Args>
		struct __TakesArgs<false, Fn, Args...> : __CallMatcher<false, std::tuple<Fn>, std::tuple<Args...>> {};


// Disable integral overflow warning on line 111 (the situation will never occur unless someone tries to call a 4 million arg function, but at that point something's wrong anyways)
		// It will never occur because the `call_matcher<arg_types, decom_type>::level` only equals `-1` if decomposition is `false` or there's a 4 million arg function being called
#pragma warning (push)
#pragma warning (disable:4307)

		// Special overload to allow for tuple decomposition
		template<class Fn, class... Args>
		struct __TakesArgs<true, Fn, std::tuple<Args...>> {
			private:
				using arg_types = typename function_traits<Fn>::arg_types;
				using decom_type = std::tuple<Args...>;
				using tuple_type = std::tuple<decom_type>;

				static constexpr bool accepts_tuple = call_matcher<arg_types, tuple_type>::value;
				static constexpr bool decomposition = call_matcher<arg_types, decom_type>::value;

			public:
				// Add `1` to a decomposition to allow for matching the tuple to be selected as the better fit
				static constexpr size_t level = decomposition ? (call_matcher<arg_types, decom_type>::level + 1) : call_matcher<arg_types, tuple_type>::level;
				static constexpr bool value = accepts_tuple || decomposition;
		};
#pragma warning (pop)


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
	struct takes_args : impl::__TakesArgs<callable<Fn>::value, Fn, impl::decay_t<Args>...> {};


	// Simple wrapper that recognizes the base case function
	template<class Fn>
	struct base_case : impl::__BaseCase<callable<Fn>::value, Fn> {};


	// Simple wrapper to test whether the object can produce a given type 
	template<class Fn, class Ret>
	struct can_produce : impl::__CanProduce<callable<Fn>::value, Fn, Ret> {};
}