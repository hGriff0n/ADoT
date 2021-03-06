#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "meta.h"

// Macros to ease resolver creation and use in templates
#define RES_CLASS template<class, class...> class												// Template definition to accept a resolution meta-struct as a template parameter
#define RES_DEF template<class Arg, class... Fns> class											// Template definition to declare a resolution meta-struct

// Template definitons for ResolverImpl classes
#define RES_IMPL template<size_t I, size_t N, class Arg, class F, class... Fns> struct 									// Base case
#define RES_IMPL_R(name) template<size_t I, size_t N, class Arg, class Curr, class F, class... Fns> \
	struct name<I, N, Arg, Curr, F, Fns...> : name<I, N, Arg, Curr, F>													// Recursive case. Inherits from the singular case for relative resolution
#define RES_IMPL_S(name) template<size_t I, size_t N, class Arg, class Curr, class F> struct name<I, N, Arg, Curr, F>	// Singular case


namespace shl {
	namespace impl {

		/*
		 * Given a pack sequence, find the location of the first `val`
		 *	Is there a way to do this with constexpr ???
		 */
		template<class T, T val, size_t N, T match, T... matches>
		struct __IndexOf {
			static constexpr size_t value = (match == val) ? N : __IndexOf<T, val, N + 1, matches...>::value;
		};

		template<class T, T val, size_t N, T match >
		struct __IndexOf<T, val, N, match> {
			static constexpr size_t value = (match == val) ? N : NOT_FOUND;
		};

		/*
		 * Helper struct for Matcher that handles all function dispatching without creating
		 *  fatal compiler errors through mutually exclusive `std::enable_if` specializations
		 *
		 *	`invoke` - Handle dispatch to the various function cases
		 *	`nice_invoke` - Gives nicer compiler errors (prevents std::get<N> from producing any) if a non-exhaustive pattern is found
		 *
		 *	Can possibly clean up SFINAE functions when `if constexpr` is implemented
		 */
		struct __MatchHelper {
			// Dispatch to the base case (callable<F> == true if base_case<F> == true)
			template<class F, class T>
			static std::enable_if_t<base_case<F>::value> invoke(F&& fn, T&& val) {
				fn();
			}

			// Dispatch to a function that accepts arguments
			template<class F, class T>
			static std::enable_if_t<!base_case<F>::value && callable<F>::value> invoke(F&& fn, T&& val) {
				fn(std::forward<T>(val));
			}

			// Apply tuple to the chosen function (only created if the function takes the decomposed tuple)
			template<class F, class... T>
			static std::enable_if_t<!base_case<F>::value && impl::takes_args<callable<F>::value, F, shl::decay_t<T>...>::value> invoke(F&& fn, std::tuple<T...> val) {
				std::apply(std::forward<F>(fn), std::forward<std::tuple<T...>>(val));
			}

			// Dispatch to a tuple pack
			// TODO: Multiple arguments are unimplemented

			// Dispatch to a non-function value
			template<class F, class T>
			static std::enable_if_t<!callable<F>::value> invoke(F&& fn, T&& val) {
				fn;
			}

			// I can remove this and the size_t template (see commented code in Matcher), but this makes nicer compiler errors
			template<size_t N, class T, class... Args>
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
		 *		 The S specialization handles the actual comparison between two functions
		 *		 The R specialization handles iteration across the list of functions
		 */
		RES_IMPL __DefaultResolverImpl {
			static constexpr auto value = I;
			using type = F;
		};

		RES_IMPL_S(__DefaultResolverImpl) {
			static constexpr bool better = better_match<Curr, F, Arg>::value;
			
			static constexpr size_t value = better ? N : I;
			using type = std::conditional_t<better, F, Curr>;
		};

		RES_IMPL_R(__DefaultResolverImpl) {
			static constexpr auto value = better ? __DefaultResolverImpl<N, N + 1, Arg, F, Fns...>::value		// The new function was a better match
				: __DefaultResolverImpl<I, N + 1, Arg, Curr, Fns...>::value;									// The old function was a better match
		};


		/*
		 * Apply the resolver struct to the reverse of a function list (and return the correct index for the non-reverse list)
		 *  Note: This can't be used directly as a `RES_CLASS` in Matcher objects (but can be used for implementations)
		 */
		template<RES_CLASS Resolver, class Arg, class... Fns>
		struct ReverseResolver {
			static constexpr auto value = sizeof...(Fns) - Resolver<Arg, Fns...>::value - 1;
		};

		template<RES_CLASS Resolver, class Arg, class... Fns>
		struct ReverseResolver<Resolver, Arg, std::tuple<Fns...>> {
			static constexpr auto value = sizeof...(Fns) - Resolver<Arg, Fns...>::value - 1;
		};
	}

	/*
	 * Default Resolver class for ADoT matching. Mirrors C++ function resolution with 2 changes
	 *	  1) Considers applying/packing tuples in order to call functions (will be generalised to variant and any in the future)
	 *	  2) Doesn't throw error if two cases have the same precedence, takes the first one instead (StrictResolver does throw error
	 */
	RES_DEF DefaultResolver {
		using res = impl::__DefaultResolverImpl<0, 1, Arg, Fns...>;

		public:
			static constexpr auto value = impl::takes_args<callable<res::type>::value, res::type, shl::decay_t<Arg>>::value ? res::value : NOT_FOUND;
	};

	/*
	* Alternate struct to determine function match ordering under the C++ standard. This resolver more closely
	*  mirrors compiler behavior in that it stops compilation if an ambiguous match resolution is found.
	*
	* Implementation follows the problem of first appearance in DefaultResolver:
	*  If reversing the order of the function list, gives a different result from DefaultResolver
	*  Then the result depended on the exact ordering of the list and therefore the resolution is ambiguous
	*/
	RES_DEF StrictResolver : public DefaultResolver<Arg, Fns...>{
		using reverse = impl::ReverseResolver<shl::DefaultResolver, Arg, typename reverse<Fns...>::type>;

		static_assert(value == reverse::value, "An ambiguous match case was found with shl::impl::StrictResolver");
	};


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

				// Find the index of the base case function 
				constexpr auto base_index = __IndexOf<bool, true, 0, base_case<Fns>::value...>::value;

				// Attempt to find a function according to the given resolver
				constexpr auto index = (Resolver<T, Fns...>::value == NOT_FOUND) ? base_index : Resolver<T, Fns...>::value;

				// Raise compiler errors if no function was found or if the match contains 18,446,744,073,709,551,615 cases (-1 is used for NOT_FOUND)
				static_assert(sizeof...(Fns) != std::numeric_limits<size_t>::max(), "Match statement contains too many cases. Please consider refactoring");
				static_assert(sizeof...(Fns) > index, "Non-exhaustive pattern match found. Resolver did not find a valid match in the case list");

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