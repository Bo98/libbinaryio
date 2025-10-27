#pragma once
#include <ios>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include "util.hpp"

#ifndef NDEBUG
#include <cassert>
#else
#include <iostream>
#endif

namespace binaryio
{
	class BinaryReader
	{
	public:
		BinaryReader(std::span<uint8_t> buffer, bool bigEndian = false);

		BinaryReader Copy() const;

		template<typename T>
		std::enable_if_t<std::is_arithmetic_v<typename SafeUnderlyingType<T>::type>, T> Read()
		{
			CheckBounds(sizeof(T));

			const auto data = m_buffer.begin() + EffectiveOffset();
			uintmax_t result = 0;

			for (auto i = 0U; i < sizeof(T); i++)
			{
				if (m_bigEndian)
					result |= static_cast<uintmax_t>(data[sizeof(T) - i - 1]) << (i * 8);
				else
					result |= static_cast<uintmax_t>(data[i]) << (i * 8);
			}

			Skip<T>();
			return reinterpret_cast<T &>(result);
		}

		template<typename T>
		std::enable_if_t<HasValueType<T>::value, T> Read()
		{
			return ReadImpl<T>(std::make_index_sequence<sizeof(T) / sizeof(typename T::value_type)>{});
		}

		template<typename T>
		std::enable_if_t<std::is_pointer_v<T>, T> Read(size_t size)
		{
			using R = std::remove_pointer_t<T>;
			T result = new R[size];

			if (sizeof(R) == 1)
			{
				CheckBounds(size);

				// Faster read.
				std::copy(m_buffer.begin() + EffectiveOffset(), m_buffer.begin() + EffectiveOffset() + size, result);
				Seek(static_cast<off_t>(size), std::ios::cur);
			}
			else
			{
				for (auto i = 0U; i < size; i++)
					result[i] = Read<R>();
			}

			return result;
		}

		template<typename T>
		std::enable_if_t<std::is_arithmetic_v<typename SafeUnderlyingType<T>::type> || HasValueType<T>::value> Verify(T comparison)
		{
			auto value = Read<T>();
			VerifyImpl(value, comparison);
		}

		template<typename T>
		void Skip()
		{
			Seek(sizeof(T), std::ios::cur);
		}

		void Seek(off_t offset, std::ios::seekdir seekDir = std::ios::beg)
		{
			if (seekDir == std::ios::cur)
				m_offset += offset;
			else
				m_offset = offset;
		}

		std::span<uint8_t> GetBuffer() const
		{
			return m_buffer;
		}

		bool IsBigEndian() const
		{
			return m_bigEndian;
		}

		void SetBigEndian(bool bigEndian)
		{
			m_bigEndian = bigEndian;
		}

		void Set64BitMode(bool in64BitMode)
		{
			m_64BitMode = in64BitMode;
		}

		off_t GetOffset() const
		{
			return m_offset;
		}

		void StashOffset()
		{
			m_stashedOffset += m_offset;
			m_offset = 0;
		}

		void Align();
		void Align(size_t byteAlignment);

		uint64_t ReadPointer();
		void SkipPointer();
		void VerifyPointer(uint64_t comparison)
		{
			VerifyImpl(ReadPointer(), comparison);
		}

		std::string ReadString();
		std::string ReadString(size_t size);

	private:
		template<typename T, size_t... Is>
		std::enable_if_t<HasValueType<T>::value, T> ReadImpl(std::index_sequence<Is...>)
		{
			return { (static_cast<void>(Is), Read<typename T::value_type>())... };
		}

		template<typename T>
		std::enable_if_t<std::is_arithmetic_v<typename SafeUnderlyingType<T>::type> || HasValueType<T>::value> VerifyImpl(T value, T comparison)
		{
#ifndef NDEBUG
			assert(value == comparison);
#else
			if (value != comparison)
			{
				std::cout << "CRITICAL: Expected ";
				std::cout.operator<<(comparison);
				std::cout << " at 0x" << std::hex << GetOffset() << std::dec << " but got ";
				std::cout.operator<<(value);
				std::cout << "." << std::endl;
			}
#endif
		}

		inline off_t EffectiveOffset()
		{
			return m_stashedOffset + m_offset;
		}

		inline void CheckBounds(size_t size)
		{
			if (EffectiveOffset() + size >= m_buffer.size())
				throw new std::out_of_range("offset exceeds size");
		}

		std::span<uint8_t> m_buffer;
		bool m_bigEndian;
		bool m_64BitMode;
		off_t m_offset;
		off_t m_stashedOffset;
	};
}
