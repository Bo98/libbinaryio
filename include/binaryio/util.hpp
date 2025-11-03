#pragma once
#include <concepts>
#include <type_traits>
#include <stddef.h>

namespace binaryio
{
	template <typename T, typename = typename std::is_enum<T>::type>
	struct SafeUnderlyingType
	{
		using type = T;
	};

	template <typename T>
	struct SafeUnderlyingType<T, std::true_type>
	{
		using type = std::underlying_type_t<T>;
	};


	template<typename T>
	concept HasValueType = requires { typename T::value_type; };

	template<typename T>
	concept HasColType = requires { typename T::col_type; };


	template<typename T>
		requires std::integral<T>
	T Align(T value, size_t byteAlignment)
	{
		return static_cast<T>(byteAlignment * ((value + (byteAlignment - 1)) / byteAlignment));
	}
}
