#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "Matcher.h"

namespace shl {
	/*
	 * Handles at-site resolution of a match object (ie. evaluation at construction, can't pass around)
	 *
	 *	NOTE: MatchResolver's pattern **must** end with a `||` call since match resolution is performed in
	 *		MatchResolver's destructor which must be called only once for correctness. This distinction is
	 *		enforced at compile time by the `||` operator and the first bool template. Currently, there is
	 *		no way of notifying the programmer at compile time of this error.
	 */
	template <bool, class T, class... Fns>
	class MatchResolver {
		private:
			T&& val;						// I don't have to worry about `val` "scope-leaking" because MatchResolver's guaranteed to use it in the current scope
			std::tuple<Fns...> match;

		public:
			constexpr MatchResolver(T&& val) : val{ std::forward<T>(val) }, match{ std::make_tuple() } {}
			constexpr MatchResolver(T&& val, std::tuple<Fns...>&& fns) : val{ std::forward<T>(val) }, match{ std::move(fns) } {}
			// Can't implement a "error" destructor because of all the temporaries (no way of enforcing a future match)

			template <class F>	
			constexpr MatchResolver<false, T, Fns..., impl::decay_t<F>> operator|(F&& fn) {
				using namespace std;
				return{ forward<T>(val), tuple_cat(move(match), make_tuple(move(fn))) };
			}

			template <class F>
			constexpr MatchResolver<true, T, Fns..., impl::decay_t<F>> operator||(F&& fn) {
				using namespace std;
				return{ forward<T>(val), tuple_cat(move(match), make_tuple(move(fn))) };
			}
	};

	/*
	 * A completed MatchResolver instance. Implements a custom destructor that forwards resolution to the Matcher object
	 */
	template <class T, class... Fns>
	class MatchResolver<true, T, Fns...> {
		private:
			T&& val;
			Matcher<Fns...> match;

		public:
			constexpr MatchResolver(T&& val, std::tuple<Fns...>&& fns) : val{ std::forward<T>(val) }, match{ std::move(fns) } {}
			~MatchResolver() { match(std::forward<T>(val)); }
	};

	// Interface function for performing a match on-site (ie. no Matcher object is exported to the scope)
	template<class T> constexpr MatchResolver<false, T> match(T&& val) {
		return std::forward<T>(val);
	}
}