#pragma once

#include <tuple>

namespace shl {
	// Preserve typing for string literals (std::decay strips away the size, etc.)
	template<class Arg>
	struct decay : std::decay<Arg> {};

	template<class C, size_t N>
	struct decay<C(&)[N]> {
		using type = C(&)[N];
	};

	template<class T>
	using decay_t = typename decay<T>::type;

	/*
	 * type_traits struct for functions and function objects
	 */
	template <class Callable>
	struct function_traits;

	// Match function pointer
	template <class R, class... Args>
	struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

	// Match raw function
	template <class R, class... Args>
	struct function_traits<R(Args...)> {
		using return_type = R;
		using arg_types = std::tuple<Args...>;
		using full_args = std::tuple<Args...>;					// This keeps the "this" type for class methods

		static constexpr size_t arity = sizeof...(Args);

		template <size_t i>
		struct arg {
			using type = std::tuple_element_t<i, arg_types>;
		};
	};

	// Match member functions
	template <class C, class R, class... Args>
	struct function_traits<R(C::*)(Args...)> : public function_traits<R(C&, Args...)> {
		using arg_types = std::tuple<Args...>;
	};

	template <class C, class R, class... Args>
	struct function_traits<R(C::*)(Args...) const> : public function_traits<R(C&, Args...)> {
		using arg_types = std::tuple<Args...>;
	};

	template <class C, class R>
	struct function_traits<R(C::*)> : public function_traits<R(C&)> {};

	// Match functors
	template <class Callable>
	struct function_traits {
		private:
			using call_type = function_traits<decltype(&Callable::operator())>;

		public:
			using return_type = typename call_type::return_type;
			using arg_types = typename call_type::arg_types;
			using full_args = typename call_type::full_args;

			static constexpr size_t arity = call_type::arity - 1;

			template <size_t i>
			struct arg {
				using type = std::tuple_element_t<i, arg_types>;
			};
	};

	// Remove & and && qualifiers
	template <class F>
	struct function_traits<F&> : public function_traits<F> {};

	template <class F>
	struct function_traits<F&&> : public function_traits<F> {};


	/*
	 * Check if the given type is Callable (operator() is defined)
	 */

	 // Match lambdas and std::function
		 // Note: Doesn't work with generic lambdas
	template <typename F>
	struct callable {
		private:
			using Yes = char;
			using No = long;

			template <typename T> static constexpr Yes is(decltype(&std::decay_t<T>::operator()));
			template <typename T> static constexpr No is(...);

		public:
			static constexpr bool value = (sizeof(is<F>(nullptr)) == sizeof(Yes));
	};

	// Match raw function
	template <typename Ret, typename... Args>
	struct callable<Ret(Args...)> : std::true_type {};

	// Match function pointer
	template <typename Ret, typename... Args>
	struct callable<Ret(*)(Args...)> : std::true_type {};


	/*
	 * Reduce a type list through folds
	 *	all: check that all types in Ts inherit from W (Note: W inherits from W)
	 *	one: check that one type in Ts inherits from W
	 */
	template<class W, class Ts>
	struct all : std::is_base_of<W, Ts> {};

	template<class W, class T, class... Ts>
	struct all<W, std::tuple<T, Ts...>>
		: std::conditional_t<std::is_base_of<W, T>::value, all<W, std::tuple<Ts...>>, std::false_type> {};

	template<class W, class T>
	struct all<W, std::tuple<T>>
		: std::conditional_t<std::is_base_of<W, T>::value, std::true_type, std::false_type> {};

	template<class W, class Ts>
	struct one : all<W, Ts> {};

	template<class W, class T, class... Ts>
	struct one<W, std::tuple<T, Ts...>>
		: std::conditional_t<std::is_base_of<W, T>::value, std::true_type, one<W, std::tuple<Ts...>>> {};

	template<class W, class T>
	struct one<W, std::tuple<T>>
		: std::conditional_t<std::is_base_of<W, T>::value, std::true_type, std::false_type> {};


	// Use less than in templates
	template<class A, class B>
	constexpr bool less() { return A::value < B::value; }


	/*
	* Impl classes to handle differing arities
	*/
	template<bool, class P, class A>
	struct callable_with_impl : std::false_type {};

	template<class... Ps, class... As>
	struct callable_with_impl<true, std::tuple<Ps...>, std::tuple<As...>> : all<std::true_type, std::tuple<std::is_convertible<As, Ps>...>> {};

	/*
	* Determine if a function can be called with the given arg types
	*/
	template<class F, class Args>
	struct callable_with : std::false_type {};

	template<class... Ps, class... As>
	struct callable_with<std::tuple<Ps...>, std::tuple<As...>> : callable_with_impl<sizeof...(Ps) == sizeof...(As), std::tuple<Ps...>, std::tuple<As...>> {};


	/*
	 * Metastructs to determine the relative ordering of two parameters according to overload resolution rules
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
	struct IsExactMatch : std::is_same<shl::decay_t<F>, shl::decay_t<T>> {};

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
}