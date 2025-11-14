#pragma once
#include <bit>
#include <ios>
#include <span>
#include <stdexcept>
#include <stdint.h>
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
		BinaryReader(std::span<uint8_t> buffer, std::endian endian = std::endian::native)
			: m_buffer(std::move(buffer)), m_offset(0), m_stashedOffset(0), m_endian(endian), m_64BitMode(false) {}

		template<typename T>
			requires std::is_arithmetic_v<typename SafeUnderlyingType<T>::type>
		[[nodiscard]] T Read()
		{
			CheckBounds(sizeof(T));

			const auto data = m_buffer.begin() + EffectiveOffset();
			uintmax_t result = 0;

			for (auto i = 0U; i < sizeof(T); i++)
			{
				if (m_endian != std::endian::native)
					result |= static_cast<uintmax_t>(data[sizeof(T) - i - 1]) << (i * 8);
				else
					result |= static_cast<uintmax_t>(data[i]) << (i * 8);
			}

			Skip<T>();
			return reinterpret_cast<T &>(result);
		}

		template<typename T>
			requires HasValueType<T>
		[[nodiscard]] T Read()
		{
			return ReadImpl<T>(std::make_index_sequence<sizeof(T) / sizeof(typename T::value_type)>{});
		}

		template<typename T>
			requires std::is_pointer_v<T>
		[[nodiscard]] T Read(size_t size)
		{
			using R = std::remove_pointer_t<T>;
			T result = new R[size];

			if constexpr (sizeof(R) == 1)
			{
				CheckBounds(size);

				// Faster read.
				std::copy(m_buffer.begin() + EffectiveOffset(), m_buffer.begin() + EffectiveOffset() + size, result);
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
			Seek(sizeof(T), std::ios::cur);
		}

		void Seek(size_t offset)
		{
			Seek(static_cast<std::streamoff>(offset), std::ios::beg);
		}

		void Seek(std::streamoff offset, std::ios::seekdir seekDir)
		{
			if (seekDir == std::ios::cur)
				offset += m_offset;
			else if (seekDir == std::ios::end)
				offset += m_buffer.size();

			if (offset < 0 || m_stashedOffset + offset > m_buffer.size())
				throw new std::out_of_range("seek out of bounds");
			
			m_offset = offset;
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
			m_offset = binaryio::Align(m_offset, byteAlignment);
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
			CheckBounds(size);
			std::string result(m_buffer.begin() + EffectiveOffset(), m_buffer.begin() + EffectiveOffset() + size);
			Seek(static_cast<std::streamoff>(size), std::ios::cur);
			return result;
		}

	private:
		template<typename T, size_t... Is>
			requires HasValueType<T>
		[[nodiscard]] T ReadImpl(std::index_sequence<Is...>)
		{
			return { (static_cast<void>(Is), Read<typename T::value_type>())... };
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

		[[nodiscard]] inline size_t EffectiveOffset() const
		{
			return m_stashedOffset + m_offset;
		}

		inline void CheckBounds(size_t size) const
		{
			if (EffectiveOffset() + size > m_buffer.size())
				throw new std::out_of_range("offset exceeds size");
		}

		std::span<uint8_t> m_buffer;
		size_t m_offset;
		size_t m_stashedOffset;
		std::endian m_endian;
		bool m_64BitMode;
	};
}
