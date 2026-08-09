#pragma once
#include <algorithm>
#include <bit>
#include <cstring>
#include <ios>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include "util.hpp" // IWYU pragma: export

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
		BinaryReader(std::span<uint8_t> buffer, std::endian endian = std::endian::native)
			: m_buffer(buffer), m_endian(endian) {}

		template<typename T>
			requires std::is_arithmetic_v<typename SafeUnderlyingType<T>::type>
		[[nodiscard]] T Read()
		{
			if constexpr (std::same_as<std::remove_cv_t<T>, bool>)
			{
				return Read<uint8_t>() != 0;
			}
			else
			{
				using U = MakeUnsignedInteger<typename SafeUnderlyingType<T>::type>;

				const auto data = CheckedSubspan(sizeof(T));

				U result;
				std::memcpy(&result, data.data(), sizeof(result));

				if constexpr (sizeof(U) > 1)
				{
					if (m_endian != std::endian::native)
					{
						U tmp = result;
						for (size_t i = 0; i < sizeof(U); ++i)
						{
							result = static_cast<U>((result << 8) | (tmp & 0xFFU));
							tmp >>= 8;
						}
					}
				}

				Skip<T>();
				return std::bit_cast<T>(result);
			}
		}

		template<typename T>
			requires FixedSizeSequence<T>
		[[nodiscard]] T Read()
		{
			return ReadImpl<T>(std::make_index_sequence<SequenceLength<T>()>{});
		}

		template<typename T>
			requires std::is_pointer_v<T>
		[[nodiscard]] T Read(size_t size)
		{
			using R = std::remove_pointer_t<T>;
			T result = new R[size];

			if constexpr (sizeof(R) == 1)
			{
				// Faster read.
				const auto data = CheckedSubspan(size);
				std::copy_n(data.begin(), size, result);
				Seek(static_cast<std::streamoff>(size), std::ios::cur);
			}
			else
			{
				for (auto i = 0U; i < size; i++)
					result[i] = Read<R>();
			}

			return result;
		}

		template<typename T>
		void Verify(T comparison)
		{
			auto value = Read<T>();
			VerifyImpl(value, comparison);
		}

		template<typename T>
		void Skip()
			requires requires { Read<T>(); }
		{
			if constexpr (FixedSizeSequence<T>)
			{
				for (size_t i = 0; i < SequenceLength<T>(); ++i)
					Skip<SequenceElement<T>>();
			}
			else
			{
				Seek(sizeof(T), std::ios::cur);
			}
		}

		void Seek(size_t offset)
		{
			if (!IsInBounds(offset))
				throw std::out_of_range("seek out of bounds");

			m_offset = offset;
		}

		void Seek(std::streamoff offset, std::ios::seekdir seekDir)
		{
			const auto end = m_buffer.size() - m_stashedOffset;

			size_t base;
			switch (seekDir)
			{
			case std::ios::beg:
				base = 0;
				break;
			case std::ios::cur:
				base = m_offset;
				break;
			case std::ios::end:
				base = end;
				break;
			default:
				throw std::invalid_argument("invalid seek direction");
			}

			if (offset >= 0)
			{
				if (std::cmp_greater(offset, end - base))
					throw std::out_of_range("seek out of bounds");

				m_offset = base + static_cast<size_t>(offset);
			}
			else
			{
				using U = std::make_unsigned_t<std::streamoff>;
				const auto magnitude = U{} - static_cast<U>(offset);
				if (magnitude > base)
					throw std::out_of_range("seek out of bounds");

				m_offset = base - static_cast<size_t>(magnitude);
			}
		}

		[[nodiscard]] std::span<uint8_t> GetBuffer() const
		{
			return m_buffer;
		}

		[[nodiscard]] std::endian GetEndian() const
		{
			return m_endian;
		}

		void SetEndian(std::endian endian)
		{
			m_endian = endian;
		}

		void SwapEndian()
		{
			m_endian = (m_endian == std::endian::little) ? std::endian::big : std::endian::little;
		}

		void Set64BitMode(bool in64BitMode)
		{
			m_64BitMode = in64BitMode;
		}

		[[nodiscard]] size_t GetOffset() const
		{
			return m_offset;
		}

		void StashOffset()
		{
			m_stashedOffset += m_offset;
			m_offset = 0;
		}

		void Align()
		{
			Align(m_64BitMode ? 8 : 4);
		}

		void Align(size_t byteAlignment)
		{
			m_offset = std::min(binaryio::Align(m_offset, byteAlignment), m_buffer.size() - m_stashedOffset);
		}

		[[nodiscard]] uint64_t ReadPointer()
		{
			Align();

			if (m_64BitMode)
				return Read<uint64_t>();

			return Read<uint32_t>();
		}

		void SkipPointer()
		{
			Align();

			if (m_64BitMode)
				return Skip<uint64_t>();

			Skip<uint32_t>();
		}

		void VerifyPointer(uint64_t comparison)
		{
			VerifyImpl(ReadPointer(), comparison);
		}

		[[nodiscard]] std::string ReadString()
		{
			std::string result;
			char c;
			while ((c = Read<char>()))
				result.push_back(c);

			Align();

			return result;
		}

		[[nodiscard]] std::string ReadString(size_t size)
		{
			const auto data = CheckedSubspan(size);
			std::string result(data.begin(), data.end());
			Seek(static_cast<std::streamoff>(size), std::ios::cur);
			return result;
		}

	private:
		template<typename T, size_t... Is>
			requires FixedSizeSequence<T>
		[[nodiscard]] T ReadImpl(std::index_sequence<Is...>)
		{
			return { (static_cast<void>(Is), Read<SequenceElement<T>>())... };
		}

		template<typename T>
			requires FixedSizeSequence<T>
		[[nodiscard]] consteval static size_t SequenceLength()
		{
			if constexpr (TupleSized<T>)
				return std::tuple_size_v<T>;
			else
				return static_cast<size_t>(T::length());
		}

		template<typename T>
			requires std::is_arithmetic_v<typename SafeUnderlyingType<T>::type> || HasValueType<T>
		void VerifyImpl(T value, T comparison)
		{
#ifndef NDEBUG
			assert(value == comparison);
#else
			if (value != comparison)
			{
				std::cerr << "CRITICAL: Expected ";
				std::cerr.operator<<(comparison);
				std::cerr << " at 0x" << std::hex << GetOffset() << std::dec << " but got ";
				std::cerr.operator<<(value);
				std::cerr << "." << std::endl;
			}
#endif
		}

		[[nodiscard]] size_t EffectiveOffset() const
		{
			return m_stashedOffset + m_offset;
		}

		[[nodiscard]] bool IsInBounds(size_t offset, size_t size = 0) const
		{
			const auto available = m_buffer.size() - m_stashedOffset;
			return offset <= available &&
				size <= available - offset;
		}

		[[nodiscard]] std::span<uint8_t> CheckedSubspan(size_t size)
		{
			if (!IsInBounds(m_offset, size))
				throw std::out_of_range("offset exceeds size");

			return m_buffer.subspan(EffectiveOffset(), size);
		}

		std::span<uint8_t> m_buffer;
		size_t m_offset{ 0 };
		size_t m_stashedOffset{ 0 };
		std::endian m_endian;
		bool m_64BitMode{ false };
	};
}
