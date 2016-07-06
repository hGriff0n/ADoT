#pragma once

#include "Matcher.h"

namespace shl {
	// An unfinished call-site match
	template <bool, class T, class... Fns>
	class MatchResolver {
		private:
			T val;
			std::tuple<Fns...> match;

		public:
			MatchResolver(T val) : val{ val }, match{ std::make_tuple() } {}
			MatchResolver(T val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}
			// Can't implement a "error" destructor because of all the temporaries

			template <class F>
			MatchResolver<false, T, Fns..., F> operator|(F&& fn) {
				return MatchResolver<false, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
			}

			// TODO: I need to find a way to warn about missing these
			template <class F>
			MatchResolver<true, T, Fns..., F> operator||(F&& fn) {
				return MatchResolver<true, T, Fns..., F>{ val, std::tuple_cat(match, std::make_tuple(fn)) };
			}
	};

	// A finished call-site match
	template <class T, class... Fns>
	class MatchResolver<true, T, Fns...> {
		private:
			T val;
			Matcher<Fns...> match;

		public:
			MatchResolver(T val, std::tuple<Fns...>&& fns) : val{ val }, match{ std::move(fns) } {}
			~MatchResolver() { match(val); }
	};

	template<class T> MatchResolver<false, const T&> match(const T& val) {
		return MatchResolver<false, const T&>{val};
	}
}