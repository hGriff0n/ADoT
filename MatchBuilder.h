#pragma once

#include "Matcher.h"

namespace shl {

	/*
	 * Builds a Matcher object that can be passed around using operator chaining to add
	 *	match patterns. A final call to `||` must be performed in order to generate the
	 *	finalized Matcher (No way of getting around this AFAIK).
	 */
	template <class... Args>
	class MatchBuilder {
		private:
			std::tuple<Args...> fns;

		public:
			constexpr MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}

			// Append the new lambda onto the current list (tuple)
			template <class F>
			constexpr MatchBuilder<Args..., F> operator|(F&& fn) {
				return MatchBuilder<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
			}

			// Return the new tuple wrapped in a Matcher object instead of a MatchBuilder
				// This syntax is possibly just a temporary measure (`\` won't compile)
			template <class F>
			constexpr Matcher<Args..., F> operator||(F&& fn) {
				return Matcher<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
			}
	};

	// Interface function for starting a MatchBuilder chain
		// Inline functions can be defined in multiple translation units iff the definition is the same
	inline constexpr MatchBuilder<> match() {
		return MatchBuilder<>{ std::make_tuple() };
	}
}