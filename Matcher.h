#pragma once

#include "function_traits.h"

namespace shl {

	// SFINAE traits classes (for matching functions based on arguments)
	template<class Fn, class... Args>
	struct takes_args : std::is_same<typename shl::function_traits<Fn>::arg_types, std::tuple<Args...>> {};

	template<class Fn>
	struct base_case : takes_args<Fn> {};


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
		 *	`call` - Handle raising compile-time errors for non-exhaustive patterns
		 *
		 *	Can clean up SFINAE functions when `if constexpr` is implemented
		 */
		template <size_t N>
		struct __MatchHelper {
			template <class F, class T>
			static std::enable_if_t<base_case<F>::value> invoke(F&& fn, T val) {
				fn();
			}

			template <class F, class T>
			static std::enable_if_t<!base_case<F>::value> invoke(F&& fn, T val) {
				fn(val);
			}
			
			// Note: The `>=` is necessary as the compiler treats a `<` as starting a new template
			template <class T, class... Args>
			static std::enable_if_t<sizeof...(Args) >= N> call(std::tuple<Args...>& fns, T val) {
				__MatchHelper<N>::invoke(std::get<N>(fns), val);
			}

			// Note: The `!` is necessary as the compiler treats a `>` as ending a template
			template <class T, class... Args>
			static std::enable_if_t<!(sizeof...(Args) >= N)> call(std::tuple<Args...>&, T) {
				static_assert(false, "Non-exhaustive pattern match found");
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

				// Find the index of the first function that either takes a `T` or is the base case (returns -1 on non-exhaustive pattern)
					// Note: There has to be a better way of doing this
				constexpr size_t index = __IndexOf<0, (takes_args<Args, T>::value || base_case<Args>::value)...>::value;

				// Attempt to invoke the correctly matched function (will raise a compiler error if none are selected)
				__MatchHelper<index>::call(fns, val);
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