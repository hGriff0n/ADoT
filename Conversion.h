#pragma once

#include "meta.h"
// Use this file to develop the conversion

template<class... Args>
using argpack = std::tuple<Args...>;

/*
* Struct to determine the match level of an argument to a function call
* Specifically this determines whether `long` or `int` is a better match
*
* C++ Conversion Ranking:
*	Standard Conversion Sequence = Worst{ Exact, Promotion, Conversion }
*  Standard Conversion Sequence is always better than a user conversion sequence
*  Standard Conversion Sequence S1 is better than Standard Conversion Sequence S2 if
*		S1 <: S2 (excluding lvalue transforms)
*		rank(S1) < rank(S2)
*		s1 binds an rvalue to an rvalue while s2 binds an lvalue to an rvalue
*		s1 binds an lvalue to a function while s2 binds an rvalue to a function
*		s1 is less cv-qualified than s2
*	User Conversion Sequence U1 is better than User Conversion Sequence U2 if
*		S(U1) is better than S(U2)
*	If two conversion sequences have the same rank
*		Conversion of pointer to bool is worse other
*		Conversion of pointer to derived to pointer to base is better than conversion of pointer to dervied to void pointer
*	Ambiguous Sequences are ranked as user conversion sequences
*/

// Testif F::operator T() exists
template<class F, class t>
struct IsUserConversion : std::false_type {};

template<class F, class T>
struct IsConversion : std::is_convertible<T, F> {};

template<class F, class T>
struct IsPromotion : std::false_type {};
template<> struct IsPromotion<float, double> : std::true_type {};
template<> struct IsPromotion<char, int> : std::true_type {};
template<> struct IsPromotion<unsigned char, int> : std::conditional_t<sizeof(int) / 2 == sizeof(char), std::true_type, std::false_type> {};
template<> struct IsPromotion<short, int> : std::true_type {};
template<> struct IsPromotion<unsigned char, unsigned int> : std::true_type {};
//wchar_t, char16_t, char32_t converted to first in int, etc.. that can hold it
//unscoped enum, type not fixed, converted to first ...
//unscoped enum, type fixed, underlying type promotion
//bit field, int or unsigned int
template<> struct IsPromotion<bool, int> : std::true_type {};

template<class F, class T>
struct IsExactMatch : std::is_same<F, T> {};

template<class F, class T>
struct ConvRank {
	// The standard says worst, but that's not working for now
	//static constexpr size_t value = IsUserConversion<F, T>::value ? 3 : IsConversion<F, T>::value ? 2 : IsPromotion<F, T>::value ? 1 : IsExactMatch<F, T>::value ? 0 : -1;
	static constexpr size_t value = IsUserConversion<F, T>::value ? 3 : IsExactMatch<F, T>::value ? 0 : IsPromotion<F, T>::value ? 1 : IsConversion<F, T>::value ? 2 : -1;
};


// Impl class to resolve matching according to c++ standard



// Classes to determine if Arg maps better/equally/both to F1_Param than to F0_Param
template<class F0_Param, class F1_Param, class Arg>
struct IsBetterArg : std::conditional_t<shl::less<ConvRank<Arg, F1_Param>, ConvRank<Arg, F0_Param>>(), std::true_type, std::false_type> {};

template<class F0_Param, class F1_Param, class Arg>
struct IsEqArg : std::conditional_t<ConvRank<Arg, F0_Param>::value == ConvRank<Arg, F1_Param>::value, std::true_type, std::false_type> {};

template<class F0_Param, class F1_Param, class Arg>
struct IsBetterOrEqArg : std::conditional_t<std::is_base_of<std::true_type, IsBetterArg<F0_Param, F1_Param, Arg>>::value
	|| std::is_base_of<std::true_type, IsEqArg<F0_Param, F1_Param, Arg>>::value, std::true_type, std::false_type> {};


// Finally determine the ranking of the overload, by memberwise forwarding to other structs ;)
template<bool, class F0, class F1, class Args>
struct BetterCallImpl : std::false_type {};

// A function is a better overload if every parameter is an equal or better match to the arguments than the previous best
	// and there is at least one parameter that is a better match to its argument than the equivalent parameter in the previous best
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterCallImpl<true, argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>>
	: std::conditional_t<shl::all<std::true_type, std::tuple<IsBetterOrEqArg<F0_Params, F1_Params, Args>...>>::value						// IsBetterOrEqArg < std::true_type for all params of f1
		&& shl::one<std::true_type, std::tuple<IsBetterArg<F0_Params, F1_Params, Args>...>>::value, std::true_type, std::false_type> {};	// IsBetterArg < std::true_type for 1+ params of f1


// Add size mismatch protection to BetterMatch
template<class F0_Params, class F1_Params, class Args>
class BetterSize : public std::false_type {};

template<class... F0_Params, class... F1_Params, class... Args>
struct BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>>
	: BetterCallImpl<sizeof...(F0_Params) == sizeof...(F1_Params) && sizeof...(F1_Params) == sizeof...(Args), argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>> {};

// Add tuple application/packing considerations
template<class F0_Params, class F1_Params, class Args>
struct BetterTuple : std::false_type {};

// Match on argument types

// f0(Args...) vs F1(Args...) on Args... <- Take best match with Args... (straight call)
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>>
	: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>> {};

// f0(tuple<Args...>) vs F1(tuple<Args...>) on Args... <- Take best match with tuple<Args...> (pack tuple)
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, argpack<Args...>>
	: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>> {};

// f0(Args...) vs F1(Args...) on tuple<Args...> <- Take best match with Args... (apply tuple)
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<F0_Params...>, argpack<F1_Params...>, argpack<std::tuple<Args...>>>
	: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>> {};

// f0(tuple<Args...>) vs f1(tuple<Args...>) on tuple<Args...> <- Take best match with tuple<Args...> (straight call)
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<std::tuple<F1_Params...>>, argpack<std::tuple<Args...>>>
	: BetterSize<argpack<F0_Params...>, argpack<F1_Params...>, argpack<Args...>> {};


// Handle tuple application and packing having a lower "rank" than straight calling

// f0(tuple<Args...>) vs F1(args...) on tuple<Args...> <- Take f1 if f0 not callable with tuple<Args...>
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, argpack<std::tuple<Args...>>>
	: std::is_base_of<std::false_type, shl::callable_with<std::tuple<F0_Params...>, std::tuple<Args...>>> {};

// f0(Args...) vs F1(tuple<Args...>) on tuple<Args...> <- Take f1 if f1 callable with tuple<Args...>
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, argpack<std::tuple<Args...>>>
	: std::is_base_of<std::true_type, shl::callable_with<std::tuple<F1_Params...>, std::tuple<Args...>>> {};

// f0(tuple<Args...>) vs F1(Args...) on Args... <- Take f1 if f1 callable with Args...
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<std::tuple<F0_Params...>>, argpack<F1_Params...>, argpack<Args...>>
	: std::is_base_of<std::true_type, shl::callable_with<argpack<F1_Params...>, argpack<Args...>>> {};

// f0(Args...) vs F1(tuple<Args...>) on Args... <- Take f1 if f0 not callable with Args...
template<class... F0_Params, class... F1_Params, class... Args>
struct BetterTuple<argpack<F0_Params...>, argpack<std::tuple<F1_Params...>>, argpack<Args...>>
	: std::is_base_of<std::false_type, shl::callable_with<argpack<F0_Params...>, argpack<Args...>>> {};


// Add callable protection to BetterMatch
template<bool, bool, class F0, class F1, class... Args>
struct BetterCall : std::false_type {};

template<class F0, class F1, class... Args>
struct BetterCall<false, true, F0, F1, Args...> : std::true_type {};

template<class F0, class F1, class... Args>
struct BetterCall<true, true, F0, F1, Args...>
	: BetterTuple<typename shl::function_traits<F0>::arg_types, typename shl::function_traits<F1>::arg_types, argpack<Args...>> {};


// Interface class. Determines if F1 is a better match than F0 for the Args
template <class F0, class F1, class... Args>
struct BetterMatch : BetterCall<shl::callable<F0>::value, shl::callable<F1>::value, F0, F1, Args...> {};

template<size_t I, size_t N, class Arg, class F, class... Fns>
struct __BetterResolverImpl {
	static constexpr auto value = I;
	using type = F;
};

template<size_t I, size_t N, class Arg, class Curr, class F, class... Fns>
struct __BetterResolverImpl<I, N, Arg, Curr, F, Fns...> : public __BetterResolverImpl<I, N, Arg, Curr, F> {
	static constexpr auto value = better ? __DefaultResolverImpl<N, N + 1, Arg, F, Fns...>::value				// The new function was a better match
		: __DefaultResolverImpl<I, N + 1, Arg, Curr, Fns...>::value;			// The old function was a better match
};

template<size_t I, size_t N, class Arg, class Curr, class F>
class __BetterResolverImpl<I, N, Arg, Curr, F> {
	static constexpr bool better = BetterMatch<Curr, F, Arg>::value;
	public:
		static constexpr size_t value = better ? N : I;
		using type = std::conditional_t<better, F, Curr>;
};


RES_DEF class BetterResolver {
	using impl = __BetterResolverImpl<0, 1, Arg, Fns...>;

	public:
		static constexpr auto value = shl::takes_args<impl::type, Arg>::value ? impl::value : NOT_FOUND;
};

// TODO: Implement ConvRank accurately
// TODO: Implement to handle value cases
// TODO: Implement `callable_with` protection
// make sure to do `callable_with<BestMatch, Args...>` to check correctness