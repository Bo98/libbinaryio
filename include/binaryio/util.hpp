#pragma once
// IWYU pragma: private
// IWYU pragma: begin_exports
#include <concepts>
#include <tuple>
#include <type_traits>
#include <stddef.h>
#include <stdint.h>
// IWYU pragma: end_exports

namespace binaryio
{
	template<typename T, typename = typename std::is_enum<T>::type>
	struct SafeUnderlyingType
	{
		using type = T;
	};

	template<typename T>
	struct SafeUnderlyingType<T, std::true_type>
	{
		using type = std::underlying_type_t<T>;
	};

	template<size_t Size>
	struct SelectUint;
	template<> struct SelectUint<1> { using type = uint8_t; };
	template<> struct SelectUint<2> { using type = uint16_t; };
	template<> struct SelectUint<4> { using type = uint32_t; };
	template<> struct SelectUint<8> { using type = uint64_t; };
#ifdef __SIZEOF_INT128__
	template<> struct SelectUint<16> { using type = __uint128_t; };
#endif

	template<typename T>
		requires std::is_arithmetic_v<T>
	using MakeUnsignedInteger = typename std::conditional_t<std::is_floating_point_v<T>, SelectUint<sizeof(T)>, std::make_unsigned<T>>::type;


	template<typename T>
	concept HasValueType = requires { typename T::value_type; };

	template<typename T>
	concept TupleSized = requires { typename std::tuple_size<T>::type; };

	template<typename T>
	concept ConstexprLength = requires { std::integral_constant<size_t, static_cast<size_t>(T::length())>{}; };

	template<typename T>
	concept Indexable = requires(const T &value, size_t index) {
		value[index];
	};

	template<typename T>
	concept FixedSizeSequence = HasValueType<T> && Indexable<T> && (TupleSized<T> || ConstexprLength<T>);

	template<typename T>
	concept LengthIndexable = Indexable<T> && requires(const T &value, size_t index) {
		{ value.length() } -> std::convertible_to<size_t>;
	};

	template<class T, class U>
	concept IsOnlyExplicitlyConvertible = !std::is_convertible_v<T, U> && requires(T t) { static_cast<U>(t); };


	template<typename T>
	using SequenceElement = std::remove_cvref_t<decltype(std::declval<const T &>()[size_t{}])>;


	template<typename T>
		requires std::integral<T>
	[[nodiscard]] T Align(T value, size_t byteAlignment)
	{
		return static_cast<T>(byteAlignment * ((value + (byteAlignment - 1)) / byteAlignment));
	}
}
