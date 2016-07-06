#pragma once

#include "function_traits.h"

namespace shl {

	// SFINAE traits classes (move to a separate file ???)
	template<class Fn, class... Args>
	struct takes_args : std::is_same<typename shl::function_traits<Fn>::arg_types, std::tuple<Args...>> {};

	template<class Fn>
	struct base_case : takes_args<Fn> {};

	// Find the index of the first bool that is true
	template<size_t N, bool... match> struct __IndexFinder;

	template<size_t N, bool match>
	struct __IndexFinder<N, match> {
		static constexpr size_t value = match ? N : -1;
	};

	template <size_t N, bool match, bool... matches>
	struct __IndexFinder<N, match, matches...> {
		static constexpr size_t value = match ? N : __IndexFinder<N + 1, matches...>::value;
	};


	namespace impl {
		// Helper for MatchImplHelper to enable dispatching to the base case
			// TODO: Is there a way to combine these two classes (__Invoker and __MatcherHelper)
		template<class F>
		struct __Invoker {
			template <class T>
			static std::enable_if_t<base_case<F>::value> invoke(F&& fn, T val) {
				fn();
			}

			template <class T>
			static std::enable_if_t<!base_case<F>::value> invoke(F&& fn, T val) {
				fn(val);
			}
		};


		// Helper for Matcher::match_impl that enables better error messages when a non-exhaustive pattern match is attempted
			// Can clean up definition once `if constexpr` is implemented
			// TODO: Can I reduce using the same trick as with __Invoker ??
		template <size_t N, bool>
		struct __MatcherHelper {
			template <class _Tuple, class T>
			static void invoke(_Tuple& fns, T val) {
				__Invoker<decltype(std::get<N>(fns))>::invoke(std::get<N>(fns), val);
			}
		};

		template <size_t N>
		struct __MatcherHelper<N, false> {
			template <class _Tuple, class T>
			static void invoke(_Tuple& fns, T val) {
				static_assert(false, "Non-exhaustive pattern match found");
			}
		};
	}

	/*
	 * Matcher encapsulates a tuple of functions and provides an interface for selecting and calling one based on a given argument
	 */
	template <class... Args>
	class Matcher {
		private:
			std::tuple<Args...> fns;

			template<class T>
			void match_impl(T val) {

				// Find the location of the first function in `fns` that either takes a `T` or is the base case (returns -1 on non-exhaustive pattern)
				// Note: There has to be a better way of doing this
				constexpr size_t index = __IndexFinder<0, (takes_args<Args, T>::value || base_case<Args>::value)...>::value;

				// Attempt to invoke the correct function (will raise an error if none are selected)
				// The second template argument is for whether the index is valid for the function tuple
				// Note: The `>=` is necessary as the compiler treats a `<` as starting a new template
				impl::__MatcherHelper<index, sizeof...(Args) >= index>::invoke(fns, val);
			}

		public:
			Matcher(std::tuple<Args...>&& fns) : fns{ fns } {}

			template<class T>
			void operator()(const T& val) {
				return match_impl<const T&>(val);
			}

			template<class T>
			void match(const T& val) {
				return match_impl<const T&>(val);
			}
	};
}