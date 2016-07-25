#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "MatchBuilder.h"

namespace shl {
	/*
	 * Handles at-site resolution of a match object (ie. evaluation at construction, can't pass around)
	 *
	 *	NOTE: MatchResolver's pattern **must** end with a `||` call since match resolution is performed in
	 *		MatchResolver's destructor which must be called only once for correctness. This distinction is
	 *		enforced at compile time by the `||` operator and the first bool template. Currently, there is
	 *		no way of notifying the programmer at compile time of this error.
	 */
	template <bool, RES_CLASS Resolver, class T, class... Fns>
	class MatchResolver {
		private:
			T&& val;						// I don't have to worry about `val` "scope-leaking" because MatchResolver's guaranteed to use it in the current scope
			MatchBuilder<Resolver, Fns...> builder;

		public:
			constexpr MatchResolver(T&& val) : val{ std::forward<T>(val) }, builder{ std::make_tuple() } {}
			constexpr MatchResolver(T&& val, MatchBuilder<Resolver, Fns...>&& fns) : val{ std::forward<T>(val) }, builder{ std::move(fns) } {}
			// Can't implement a "error" destructor because of all the temporaries (no way of enforcing a future match)

			template <class F>	
			constexpr MatchResolver<false, Resolver, T, Fns..., impl::decay_t<F>> operator|(F&& fn) {
				return{ std::forward<T>(val), builder | std::move(fn) };
			}

			template <class F>
			constexpr MatchResolver<true, Resolver, T, Fns..., impl::decay_t<F>> operator||(F&& fn) {
				return{ std::forward<T>(val), builder || std::move(fn) };
			}
	};

	/*
	 * A completed MatchResolver instance. Implements a custom destructor that forwards resolution to the Matcher object
	 */
	template <RES_CLASS Resolver, class T, class... Fns>
	class MatchResolver<true, Resolver, T, Fns...> {
		private:
			T&& val;
			Matcher<Resolver, Fns...> match;

		public:
			constexpr MatchResolver(T&& val, Matcher<Resolver, Fns...>&& fns) : val{ std::forward<T>(val) }, match{ std::move(fns) } {}
			~MatchResolver() { match(std::forward<T>(val)); }
	};

	// Interface function for performing a match on-site (ie. no Matcher object is exported to the scope)
	template<RES_CLASS Resolver = impl::__CppResolver, class T> constexpr MatchResolver<false, Resolver, T> match(T&& val) {
		return std::forward<T>(val);
	}
}