#pragma once
#include <type_traits>
#include <cstdint>

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


	template<typename T, typename = void>
	struct HasValueType : std::false_type
	{
	};

	template<typename T>
	struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type
	{
	};


	template<typename T, typename = void>
	struct HasColType : std::false_type
	{
	};

	template<typename T>
	struct HasColType<T, std::void_t<typename T::col_type>> : std::true_type
	{
	};


	template<typename T>
	std::enable_if_t<std::is_integral_v<T>, T> Align(T value, size_t byteAlignment)
	{
		return static_cast<T>(byteAlignment * ((value + (byteAlignment - 1)) / byteAlignment));
	}
}
