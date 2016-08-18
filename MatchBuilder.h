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

			template<class F>
			//constexpr std::tuple<Args..., shl::decay_t<F>>&& add(F&& fn) {
			constexpr auto add_case(F&& fn) {
				return std::tuple_cat(std::move(fns), std::make_tuple(std::move(fn)));
			}

		public:
			constexpr MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}
			constexpr MatchBuilder(MatchBuilder<Resolver, Args...>&& m) : fns{ std::move(m.fns) } {}

			// Append the new case onto the current list 
			template <class F>
			constexpr MatchBuilder<Resolver, Args..., shl::decay_t<F>> operator|(F&& fn) {
				return add_case(fn);
			}

			// Add the new case to the current list and return the list as a Matcher instead of a MatchBuilder
				// This syntax is possibly just a temporary measure (`\` won't compile)
			template <class F>
			constexpr Matcher<Resolver, Args..., shl::decay_t<F>> operator||(F&& fn) {
				return add_case(fn);
			}

			// Delete copy and assignment functions to prevent compilation without a ending operator|| call
			constexpr MatchBuilder(const MatchBuilder&) = delete;
			constexpr MatchBuilder& operator=(const MatchBuilder&) = delete;
	};

	// Interface function for starting a MatchBuilder chain
	template<RES_CLASS Resolver = DefaultResolver>
	constexpr MatchBuilder<Resolver> match() {
		return std::make_tuple();
	}
}