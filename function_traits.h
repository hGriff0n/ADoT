#pragma once

#include <tuple>

namespace shl {

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

}