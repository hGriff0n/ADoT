#pragma once

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
			const T& val;
			std::tuple<Fns...> match;

		public:
			constexpr MatchResolver(const T& val) : val{ val }, match{ std::make_tuple() } {}
			constexpr MatchResolver(const T& val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}
			// Can't implement a "error" destructor because of all the temporaries (no way of enforcing a future match)

			template <class F>
			constexpr MatchResolver<false, T, Fns..., F> operator|(F&& fn) {
				return MatchResolver<false, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
			}

			template <class F>
			constexpr MatchResolver<true, T, Fns..., F> operator||(F&& fn) {
				return MatchResolver<true, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
			}
	};

	/*
	 * A completed MatchResolver instance. Implements a custom destructor that forwards resolution to the Matcher object
	 */
	template <class T, class... Fns>
	class MatchResolver<true, T, Fns...> {
		private:
			const T& val;
			Matcher<Fns...> match;

		public:
			constexpr MatchResolver(const T& val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}
			~MatchResolver() { match(val); }
	};

	// Interface function for performing a match on-site (ie. no Matcher object is exported to the scope)
	template<class T> constexpr MatchResolver<false, T> match(const T& val) {
		return MatchResolver<false, T>{ val };
	}
}