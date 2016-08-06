#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "meta.h"

#define RES_CLASS template<class, class...> class												// Template definition to accept a resolution meta-struct as a template parameter
#define RES_DEF template<class Arg, class... Fns>												// Template definition to declare a resolution meta-struct

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

		template<class T, T val, size_t N, T match >
		struct __IndexOf<T, val, N, match> {
			static constexpr size_t value = (match == val) ? N : NOT_FOUND;
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
		 * Struct to determine the best function to match the arguments according to C++ function resolution rules
		 * This struct is designed in such a way to be used to recursively "iterate" over the possible functions
		 *	to simplify the process of determining the "most suitable" function (don't have to reduce to a number)
		 *
		 * Note: The default specification handles the singular function case (ie. sizeof...(Fns) == 0)
		 *		 The first specialization handles the actual iteration among several functions
		 *		 The second specialization handles the actual resolution comparison
		 */
		template<size_t I, size_t N, class Arg, class F, class... Fns>
		struct __DefaultResolverImpl {
			static constexpr auto value = I;
			using type = F;
		};

		template<size_t I, size_t N, class Arg, class Curr, class F, class... Fns>
		struct __DefaultResolverImpl<I, N, Arg, Curr, F, Fns...> : public __DefaultResolverImpl<I, N, Arg, Curr, F> {
			static constexpr auto value = better ? __DefaultResolverImpl<N, N + 1, Arg, F, Fns...>::value				// The new function was a better match
												 : __DefaultResolverImpl<I, N + 1, Arg, Curr, Fns...>::value;			// The old function was a better match
		};

		template<size_t I, size_t N, class Arg, class Curr, class F>
		class __DefaultResolverImpl<I, N, Arg, Curr, F> {
			protected:
				// Helper members to simplify better calculation
				static constexpr size_t take_level_F = takes_args<F, Arg>::level;		// If take(F) < take(Curr), then F is automatically a better function choice
				static constexpr size_t take_level_C = takes_args<Curr, Arg>::level;	// Otherwise if take(F) = take(Curr)
				//static constexpr size_t arg_rank_F = match_rank<F, Arg>::value;		// Then match(F) < match(Curr) means that F is the better function choice
				//static constexpr size_t arg_rank_C = match_rank<Curr, Arg>::value;	// Where match(F) determines C++ resolution matching <- TODO: Improve

				// Actual better definition
				//static constexpr bool better = (take_level_F < take_level_C) || (take_level_F == take_level_C && arg_rank_F < arg_rank_C);
				static constexpr bool better = (take_level_F < take_level_c) || (take_level_F == take_level_C && false);

			public:
				static constexpr auto value = better ? N : I;
				using type = std::conditional_t<better, F, Curr>;
		};
		
		// Actual interface struct for c++ resolution. Adds correct index production as expected by Matcher
		RES_DEF class DefaultResolver {
			using impl = __DefaultResolverImpl<0, 1, Arg, Fns...>;

			public:
				static constexpr auto value = takes_args<impl::type, Arg>::value ? impl::value : NOT_FOUND;			// Check that the chosen function works (`__DefaultResolverImpl` can't produce "not found")
		};


		template<size_t I, size_t N, class Arg, class F, class... Fns>
		struct __BetterResolverImpl {
			static constexpr auto value = I;
			using type = F;
		};

		template<size_t I, size_t N, class Arg, class Curr, class F, class... Fns>
		struct __BetterResolverImpl<I, N, Arg, Curr, F, Fns...> : public __BetterResolverImpl<I, N, Arg, Curr, F> {
			static constexpr auto value = better ? __DefaultResolverImpl<N, N + 1, Arg, F, Fns...>::value		// The new function was a better match
				: __DefaultResolverImpl<I, N + 1, Arg, Curr, Fns...>::value;									// The old function was a better match
		};

		template<size_t I, size_t N, class Arg, class Curr, class F>
		class __BetterResolverImpl<I, N, Arg, Curr, F> {
			static constexpr bool better = BetterMatch<Curr, F, Arg>::value;
			public:
				static constexpr size_t value = better ? N : I;
				using type = std::conditional_t<better, F, Curr>;
		};


		RES_DEF class BetterResolver {
			using impl = __BetterResolverImpl<0, 1, Arg, Fns...>;

			public:
				static constexpr auto value = shl::takes_args<impl::type, Arg>::value ? impl::value : NOT_FOUND;
		};
		// TODO: Add strict resolver that asserts when ambiguous overload found (ie. f(long) and f(short), Default takes first)
	}

	/*
	 * Handles execution of match statement by selecting a function from a list based on argument type matching.
	 *	Matcher doesn't destroy it's function list when matching against a passed value allowing it to be reused
	 *	multiple times if desired without errors.
	 */
	template<RES_CLASS Resolver, class... Fns>
	class Matcher {
		private:
			std::tuple<Fns...> fns;

			template<class T>
			void match_impl(T&& val) {
				using namespace impl;

				// Find the index of the first function that either takes a `T` or is the base case
				constexpr auto index = (Resolver<T, Fns...>::value == NOT_FOUND) ? __IndexOf<bool, true, 0, base_case<Fns>::value...>::value : Resolver<T, Fns...>::value;

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
	template<RES_CLASS Resolver, class T, class... Args>
	void match(T&& val, Matcher<Resolver, Args...>& matcher) {
		matcher.match(std::forward<T>(val));
	}
}