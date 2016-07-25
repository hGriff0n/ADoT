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
			//constexpr std::tuple<Args..., impl::decay_t<F>>&& add(F&& fn) {
			constexpr auto add_case(F&& fn) {
				using namespace std;
				return tuple_cat(move(fns), make_tuple(move(fn)));
			}

		public:
			constexpr MatchBuilder(std::tuple<Args...>&& fns) : fns{ fns } {}
			constexpr MatchBuilder(MatchBuilder<Resolver, Args...>&& m) : fns{ std::move(m.fns) } {}

			// Append the new lambda onto the current list (tuple)
			template <class F>
			constexpr MatchBuilder<Resolver, Args..., impl::decay_t<F>> operator|(F&& fn) {
				return add_case(fn);
			}

			// Return the new tuple wrapped in a Matcher object instead of a MatchBuilder
				// This syntax is possibly just a temporary measure (`\` won't compile)
			template <class F>
			constexpr Matcher<Resolver, Args..., impl::decay_t<F>> operator||(F&& fn) {
				return add_case(fn);
			}

			// Delete copy and assignment functions to prevent compilation without a ending operator|| call
			constexpr MatchBuilder(const MatchBuilder&) = delete;
			constexpr MatchBuilder& operator=(const MatchBuilder&) = delete;
	};

	// Interface function for starting a MatchBuilder chain
	template<RES_CLASS Resolver = impl::__CppResolver>
	constexpr MatchBuilder<Resolver> match() {
		return std::make_tuple();
	}
}