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
	namespace impl {
		template<class... Args> using argpack = std::tuple<Args...>;
		

		/*
		 * Impl class to allow safe handling of non-callable types
		 *  and to add consideration of tuple decomposition
		 */
		template<bool, class Fn, class... Args>
		struct takes_args : callable_with<typename function_traits<Fn>::arg_types, std::tuple<Args...>> {};

		template<class Fn, class... Args>
		struct takes_args<false, Fn, Args...> : std::false_type {};

		// Special overload to allow for tuple decomposition
		template<class Fn, class... Args>
		struct takes_args<true, Fn, argpack<Args...>> {
			private:
				using arg_types = typename function_traits<Fn>::arg_types;
				using decom_type = argpack<Args...>;
				using tuple_type = argpack<decom_type>;

			public:
				static constexpr bool value = callable_with<arg_types, decom_type>::value || callable_with<arg_types, tuple_type>::value;
		};


		/*
		 * Metastructs to determine the relative ranking of two functions to an argument list
		 */

		// Finally determine the ranking of the overload, by memberwise forwarding to other structs ;)
		template<bool, class F0, class F1, class... Args>
		struct __BetterMatchImpl : std::false_type {};

		// A function is a better overload if every parameter is an equal or better match to the arguments than the previous best
		// and there is at least one parameter that is a better match to its argument than the equivalent parameter in the previous best
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __BetterMatchImpl<true, argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: std::conditional_t<all<std::true_type, std::tuple<IsBetterOrEqArg<F0_Params, F1_Params, Args>...>>::value						// IsBetterOrEqArg < std::true_type for all params of f1
			&& one<std::true_type, std::tuple<IsBetterArg<F0_Params, F1_Params, Args>...>>::value, std::true_type, std::false_type> {};		// IsBetterArg < std::true_type for 1+ params of f1


		/*
		 * Add size mismatch protection to __BetterMatch
		 */
		template<class F0_Params, class F1_Params, class... Args>
		class __SizeFilter : public std::false_type {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __SizeFilter<argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: __BetterMatchImpl<sizeof...(F0_Params) == sizeof...(F1_Params) && sizeof...(F1_Params) == sizeof...(Args), argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};


		/*
		 * Add tuple application/packing considerations
		 */
		template<class F0_Params, class F1_Params, class... Args>
		struct __TupleDispatch : std::false_type {};

		// Match on argument types

		// f0(Args...) vs F1(Args...) on Args... <- Take best match with Args... (straight call)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, Args...>
			: __SizeFilter<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(tuple<Args...>) vs F1(tuple<Args...>) on Args... <- Take best match with tuple<Args...> (pack tuple)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, Args...>
			: __SizeFilter<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(Args...) vs F1(Args...) on tuple<Args...> <- Take best match with Args... (apply tuple)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>>
			: __SizeFilter<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};

		// f0(tuple<Args...>) vs f1(tuple<Args...>) on tuple<Args...> <- Take best match with tuple<Args...> (straight call)
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>>
			: __SizeFilter<argpack<F0_Params...>, argpack<F1_Params...>, Args...> {};


		// Add cv-ref qualification to tuples (doesn't consider it during overloading just yet though
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>&>
			: __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>&>
			: __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>&&>
			: __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>&&>
			: __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, const std::tuple<Args...>&>
			: __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, const std::tuple<Args...>&>
			: __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, const std::tuple<Args...>&&>
			: __TupleDispatch<argpack<F0_Params...>, argpack<F1_Params...>, std::tuple<Args...>> {};

		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, const std::tuple<Args...>&&>
			: __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>> {};


		// Handle tuple application and packing having a lower "rank" than straight calling

		// f0(tuple<Args...>) vs F1(args...) on tuple<Args...> <- Take f1 if f0 not callable with tuple<Args...>
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, std::tuple<Args...>>
			: std::is_base_of<std::false_type, callable_with<std::tuple<F0_Params...>, std::tuple<Args...>>> {};

		// f0(Args...) vs F1(tuple<Args...>) on tuple<Args...> <- Take f1 if f1 callable with tuple<Args...>
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, std::tuple<Args...>>
			: std::is_base_of<std::true_type, callable_with<std::tuple<F1_Params...>, std::tuple<Args...>>> {};

		// f0(tuple<Args...>) vs F1(Args...) on Args... <- Take f1 if f1 callable with Args...
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, Args...>
			: std::is_base_of<std::true_type, callable_with<argpack<F1_Params...>, argpack<Args...>>> {};

		// f0(Args...) vs F1(tuple<Args...>) on Args... <- Take f1 if f0 not callable with Args...
		template<class... F0_Params, class... F1_Params, class... Args>
		struct __TupleDispatch<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, Args...>
			: std::is_base_of<std::false_type, callable_with<argpack<F0_Params...>, argpack<Args...>>> {};


		/*
		 * Add callable protection to __BetterMatch
		 */
		template<bool, bool, class F0, class F1, class... Args>
		struct __CallableFilter : std::false_type {};

		template<class F0, class F1, class... Args>
		struct __CallableFilter<false, true, F0, F1, Args...> : std::true_type {};

		template<class F0, class F1, class... Args>
		struct __CallableFilter<true, true, F0, F1, Args...>
			: __TupleDispatch<typename function_traits<F0>::arg_types, typename function_traits<F1>::arg_types, Args...> {};

		
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


	// Simple wrapper that recognizes the base case function
	template<class Fn>
	struct base_case : impl::__BaseCase<callable<Fn>::value, Fn> {};


	// Simple wrapper to test whether the object can produce a given type 
	template<class Fn, class Ret>
	struct can_produce : impl::__CanProduce<callable<Fn>::value, Fn, Ret> {};


	// Interface class. Determines if F1 is a better match than F0 for the Args
	template<class F0, class F1, class... Args>
	struct better_match : impl::__CallableFilter<callable<F0>::value, callable<F1>::value, F0, F1, Args...> {};
}