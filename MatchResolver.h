#pragma once
#pragma warning (disable:4814)				// Disable the c++14 warning about "constexpr not implying const"

#include "MatchBuilder.h"

namespace shl {
	/*
	 * Handles at-site resolution of a match object (ie. evaluation at construction, can't pass around)
	 *
	 *	NOTE: MatchResolver's pattern **must** end with a `||` call since match resolution is performed there.
	 *		Currently, there is no way of notifying the programmer at compile time if they've forgotten the `||`.
	 */
	template<RES_CLASS Resolver, class T, class... Fns>
	class MatchResolver {
		private:
			T&& val;						// I don't have to worry about `val` "scope-leaking" because MatchResolver's guaranteed to use it in the current scope (if the constructors are deleted)
			MatchBuilder<Resolver, Fns...> builder;

		public:
			constexpr MatchResolver(T&& val) : val{ std::forward<T>(val) }, builder{ std::make_tuple() } {}
			constexpr MatchResolver(T&& val, MatchBuilder<Resolver, Fns...>&& fns) : val{ std::forward<T>(val) }, builder{ std::move(fns) } {}
			// Can't implement an "error" destructor because of all the temporaries (no way of enforcing a future match)

			template<class F>
			constexpr MatchResolver<Resolver, T, Fns..., shl::decay_t<F>> operator|(F&& fn) {
				return{ std::forward<T>(val), builder | fn };			// Should I `forward` or `move` the function
			}

			// Handle resolution immediately once the `||` operator is used
			template<class F>
			constexpr auto operator||(F&& fn) {
				return (builder || fn).match(std::forward<T>(val));
			}


			// Comment out to allow for "match-currying" (I need to fix `val` to ensure no leakage first)
			constexpr MatchResolver(MatchResolver&&) = delete;
			constexpr MatchResolver(const MatchResolver&) = delete;
			constexpr MatchResolver& operator=(const MatchResolver&) = delete;
	};

	// Interface function for performing a match on-site (ie. no Matcher object is exported to the scope)
	template<RES_CLASS Resolver = DefaultResolver, class T> constexpr MatchResolver<Resolver, T> match(T&& val) {
		return std::forward<T>(val);
	}
}