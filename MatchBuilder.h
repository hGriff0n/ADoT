#pragma once

#include "Matcher.h"

namespace shl {
	// class that constructs a Matcher using similar syntax to a MatchResolver
	template <class... Args>
	class MatchBuilder {
		private:
			std::tuple<Args...> fns;

		public:
			MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}

			// Append the new lambda onto the current list (tuple)
			template <class F>
			MatchBuilder<Args..., F> operator|(F&& fn) {
				return MatchBuilder<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
			}

			// Return the new tuple wrapped in a Matcher object instead of a MatchBuilder
				// This syntax is possibly just a temporary measure
				// though `\` doesn't compile for some reason
			template <class F>
			Matcher<Args..., F> operator||(F&& fn) {
				return Matcher<Args..., F>(std::tuple_cat(fns, std::make_tuple(std::move(fn))));
			}
			// This almost fucked me over after I changed Matcher's template from `_Tuple` to `Args...`
				// I'd accidentally forgot to change the return type from `Matcher<std::tuple<Args..., F>>` to `Matcher<Args..., F>` but it still compiled and ran
	};


	MatchBuilder<> match();
}