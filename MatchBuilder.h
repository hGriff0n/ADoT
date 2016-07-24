#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "Matcher.h"

namespace shl {

	/*
	 * Builds a Matcher object that can be passed around using operator chaining to add
	 *	match patterns. A final call to `||` must be performed in order to generate the
	 *	finalized Matcher (No way of getting around this AFAIK).
	 */
	template <RES_CLASS Resolver, class... Args>
	class MatchBuilder {
		private:
			std::tuple<Args...> fns;

		public:
			constexpr MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}

			// Append the new lambda onto the current list (tuple)
			template <class F>
			constexpr MatchBuilder<Resolver, Args..., impl::decay_t<F>> operator|(F&& fn) {
				return std::tuple_cat(std::move(fns), std::make_tuple(std::move(fn)));
			}

			// Return the new tuple wrapped in a Matcher object instead of a MatchBuilder
				// This syntax is possibly just a temporary measure (`\` won't compile)
			template <class F>
			constexpr Matcher<Resolver, Args..., impl::decay_t<F>> operator||(F&& fn) {
				return std::tuple_cat(std::move(fns), std::make_tuple(std::move(fn)));
			}
	};

	// Interface function for starting a MatchBuilder chain
	template<RES_CLASS Resolver = impl::__CppResolver>
	constexpr MatchBuilder<Resolver> match() {
		return std::make_tuple();
	}
}