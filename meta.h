#pragma once

#include "function_traits.h"

#define NOT_FOUND -1

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
		// Preserve typing for string literals (std::decay strips away the size, etc.)
		template<class Arg>
		struct decay : std::decay<Arg> {};

		template<class C, size_t N>
		struct decay<C(&)[N]> {
			using type = C(&)[N];
		};

		template<class T>
		using decay_t = typename decay<T>::type;

		// Deprecated code begins

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
			static constexpr size_t level = value ? sizeof...(Params)-num_true<std::is_same<impl::decay_t<Params>, Args>::value...>::value : NOT_FOUND;
		};

		template<class... Params, class... Args>
		struct __CallMatcher<false, std::tuple<Params...>, std::tuple<Args...>> {
			static constexpr bool value = false;
			static constexpr size_t level = NOT_FOUND;				// A level of 0 indicates a perfect match
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

		// Deprecated code ends

		/*
		 * Metastructs to determine the relative matching of two parameters to an argument 
		 */
		// Test if F -> T is possible through user conversion operator
		template<class T, class F>
		struct IsUserConvertable : std::false_type {};

		// Test if F -> T is possible through user conversion operator
		template<class T, class F>
		struct IsStdConvertable : std::is_convertible<F, T> {};

		// Test if F -> T is a numeric promotion
		template<class F, class T>
		struct IsPromotion : std::false_type {};
		template<> struct IsPromotion<float, double> : std::true_type {};
		template<> struct IsPromotion<char, int> : std::true_type {};
		template<> struct IsPromotion<short, int> : std::true_type {};
		template<> struct IsPromotion<bool, int> : std::true_type {};
		// TODO: Complete implementation (unsigned, whcar_t, enums, bit fields)

		template<class F, class T>
		struct IsExactMatch : std::is_same<impl::decay_t<F>, impl::decay_t<T>> {};

		template<class F, class T>
		struct ConvRank {
			// The standard says worst, but that's not working for now
			//static constexpr size_t value = IsUserConvertable<F, T>::value ? 3 : IsStdConvertable<F, T>::value ? 2 : IsPromotion<F, T>::value ? 1 : IsExactMatch<F, T>::value ? 0 : -1;
			static constexpr size_t value = IsUserConvertable<F, T>::value ? 3 : IsExactMatch<F, T>::value ? 0 : IsPromotion<F, T>::value ? 1 : IsStdConvertable<F, T>::value ? 2 : -1;
		};

		template<class F0_Param, class F1_Param, class Arg>
		struct IsBetterArg : std::conditional_t<less<ConvRank<Arg, F1_Param>, ConvRank<Arg, F0_Param>>(), std::true_type, std::false_type> {};

		template<class F0_Param, class F1_Param, class Arg>
		struct IsEqArg : std::conditional_t<ConvRank<Arg, F0_Param>::value == ConvRank<Arg, F1_Param>::value, std::true_type, std::false_type> {};

		template<class F0_Param, class F1_Param, class Arg>
		struct IsBetterOrEqArg : std::conditional_t<std::is_base_of<std::true_type, IsBetterArg<F0_Param, F1_Param, Arg>>::value
			|| std::is_base_of<std::true_type, IsEqArg<F0_Param, F1_Param, Arg>>::value, std::true_type, std::false_type> {};

		/*
		 * Metastructs to determine the relative ranking of two functions to an argument list
		 */
		template<class... Args> using argpack = std::tuple<Args...>;
		// Finally determine the ranking of the overload, by memberwise forwarding to other structs ;)
		template<bool, class F0, class F1, class... Args>
		struct BetterCallImpl : std::false_type {};

		// A function is a better overload if every parameter is an equal or better match to the arguments than the previous best
		// and there is at least one parameter that is a better match to its argument than the equivalent parameter in the previous best
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterCallImpl<true, argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: std::conditional_t<all<std::true_type, std::tuple<IsBetterOrEqArg<F0_Params, F1_Params, Args>...>>::value						// IsBetterOrEqArg < std::true_type for all params of f1
			&& one<std::true_type, std::tuple<IsBetterArg<F0_Params, F1_Params, Args>...>>::value, std::true_type, std::false_type> {};		// IsBetterArg < std::true_type for 1+ params of f1

		// Add size mismatch protection to BetterMatch
		template<class F0_Params, class F1_Params, class... Args>
		class BetterSize : public std::false_type {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: BetterCallImpl<sizeof...(F0_Params) == sizeof...(F1_Params) && sizeof...(F1_Params) == sizeof...(Args), argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};


		// Add tuple application/packing considerations
		template<class F0_Params, class F1_Params, class... Args>
		struct BetterTuple : std::false_type {};

		// Match on argument types

		// f0(Args...) vs F1(Args...) on Args... <- Take best match with Args... (straight call)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(tuple<Args...>) vs F1(tuple<Args...>) on Args... <- Take best match with tuple<Args...> (pack tuple)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, Args...>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(Args...) vs F1(Args...) on tuple<Args...> <- Take best match with Args... (apply tuple)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(tuple<Args...>) vs f1(tuple<Args...>) on tuple<Args...> <- Take best match with tuple<Args...> (straight call)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};


		// Add cv-ref qualification to tuples (doesn't consider it during overloading just yet though
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>&>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>&>
			: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};


		// Handle tuple application and packing having a lower "rank" than straight calling

		// f0(tuple<Args...>) vs F1(args...) on tuple<Args...> <- Take f1 if f0 not callable with tuple<Args...>
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, std::tuple<Args...>>
			: std::is_base_of<std::false_type, callable_with<std::tuple<F0_Params...>, std::tuple<Args...>>> {};

		// f0(Args...) vs F1(tuple<Args...>) on tuple<Args...> <- Take f1 if f1 callable with tuple<Args...>
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>>
			: std::is_base_of<std::true_type, callable_with<std::tuple<F1_Params...>, std::tuple<Args...>>> {};

		// f0(tuple<Args...>) vs F1(Args...) on Args... <- Take f1 if f1 callable with Args...
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, Args...>
			: std::is_base_of<std::true_type, callable_with<argpack<F1_Params...>, argpack<Args...>>> {};

		// f0(Args...) vs F1(tuple<Args...>) on Args... <- Take f1 if f0 not callable with Args...
		template<class... F0_Params, class... F1_Params, class... Args>
		struct BetterTuple<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, Args...>
			: std::is_base_of<std::false_type, callable_with<argpack<F0_Params...>, argpack<Args...>>> {};


		// Add callable protection to BetterMatch
		template<bool, bool, class F0, class F1, class... Args>
		struct BetterCall : std::false_type {};

		template<class F0, class F1, class... Args>
		struct BetterCall<false, true, F0, F1, Args...> : std::true_type {};

		template<class F0, class F1, class... Args>
		struct BetterCall<true, true, F0, F1, Args...>
			: BetterTuple<typename function_traits<F0>::arg_types, typename function_traits<F1>::arg_types, Args...> {};


		// Interface class. Determines if F1 is a better match than F0 for the Args
		template <class F0, class F1, class... Args>
		struct BetterMatch : BetterCall<callable<F0>::value, callable<F1>::value, F0, F1, Args...> {};


		
		// Impl class to enable safe handling of types that aren't callable
		template<bool, class Fn>
		struct __BaseCase {
			static constexpr bool value = function_traits<Fn>::arity == 0;
		};
		template<class Fn> struct __BaseCase<false, Fn> : std::false_type {};


		// Impl class to enable handling of non-function types 
		template<bool, class Fn, class Ret>
		struct __CanProduce : std::is_convertible<typename function_traits<Fn>::ret_type, Ret> {};

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