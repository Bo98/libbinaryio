#pragma once
#include <array>
#include <bit>
#include <functional>
#include <ios>
#include <memory>
#include <queue>
#include <ranges>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string_view>
#include <utility>
#include "util.hpp" // IWYU pragma: export

namespace binaryio
{
	template<typename T>
	concept StringViewConvertible = std::convertible_to<const T &, std::string_view>;

	class BinaryWriter
	{
	public:
		BinaryWriter()
		{
			m_outStream.exceptions(std::ios::badbit | std::ios::failbit);
		}

		template<typename T>
			requires std::is_arithmetic_v<typename SafeUnderlyingType<T>::type>
		void Write(T value)
		{
			if constexpr (sizeof(T) > 1)
			{
				if (m_endian != std::endian::native)
				{
					union {
						T val;
						std::array<uint8_t, sizeof(T)> bytes;
					} processedValue = { .val = value };

					for (auto i = 0U; i < sizeof(T) / 2; i++)
					{
						const auto tmp = processedValue.bytes[sizeof(T) - i - 1];
						processedValue.bytes[sizeof(T) - i - 1] = processedValue.bytes[i];
						processedValue.bytes[i] = tmp;
					}

					value = processedValue.val;
				}
			}

			m_outStream.write(reinterpret_cast<const char *>(&value), sizeof(T));
		}

		template<typename T>
			requires(std::ranges::input_range<const T> && !StringViewConvertible<T>)
		void Write(const T &values)
		{
			for (const auto &value : values)
				Write(value);
		}

		template<typename T>
			requires(!std::ranges::input_range<const T> && LengthIndexable<T> && !StringViewConvertible<T>)
		void Write(const T &value)
		{
			for (size_t i = 0; i < static_cast<size_t>(value.length()); i++)
				Write(value[i]);
		}

		template<typename T, typename U>
			requires std::is_arithmetic_v<typename SafeUnderlyingType<T>::type> && IsOnlyExplicitlyConvertible<U, T>
		void Write(U value)
		{
			return Write(static_cast<T>(value));
		}

		void Write(std::string_view value, bool nullTerminate = true)
		{
			if (!value.empty())
				m_outStream.write(value.data(), static_cast<std::streamsize>(value.length()));

			if (nullTerminate)
				Write<uint8_t>(0);
		}

		template<typename T>
			requires std::is_pointer_v<T>
		void Write(T value, size_t elementCount)
		{
			if constexpr (sizeof(std::remove_pointer_t<T>) == 1)
			{
				m_outStream.write(reinterpret_cast<const char *>(value), static_cast<std::streamsize>(elementCount));
			}
			else
			{
				for (auto i = 0U; i < elementCount; i++)
					Write(value[i]);
			}
		}

		template<typename T>
		void VisitAndWrite(size_t &offset, T value)
		{
			const auto prevPos = GetOffset();
			Seek(offset);
			Write(value);

			offset = binaryio::Align(GetOffset(), 4);

			Seek(prevPos);
		}

		void Append(BinaryWriter &writer)
		{
			if (writer.GetSize() == 0)
				return;

			writer.m_outStream.seekg(0);
			Seek(0, std::ios::end);
			m_outStream << writer.m_outStream.rdbuf();
		}

		void Defer(const std::function<void(BinaryWriter &writer)> &deferWriteFn)
		{
			m_deferredWrites.push(deferWriteFn);
		}

		void ProcessDeferQueue()
		{
			while (!m_deferredWrites.empty())
			{
				m_deferredWrites.front()(*this);
				m_deferredWrites.pop();
			}
		}

		void PushAndClearDeferQueue()
		{
			m_pushedDeferredWrites.push(std::move(m_deferredWrites));
			m_deferredWrites = {};
		}

		void PopDeferQueue()
		{
			m_deferredWrites = std::move(m_pushedDeferredWrites.top());
			m_pushedDeferredWrites.pop();
		}

		void Seek(size_t offset)
		{
			Seek(static_cast<std::streamoff>(offset), std::ios::beg);
		}

		void Seek(std::streamoff offset, std::ios::seekdir seekdir)
		{
			auto absoluteOffset = offset;
			if (seekdir == std::ios::cur)
				absoluteOffset += m_outStream.tellp();
			else if (seekdir == std::ios::end)
				absoluteOffset += static_cast<std::streamoff>(GetSize());

			if (absoluteOffset < 0)
				throw std::out_of_range("seek out of bounds");

			if (static_cast<size_t>(absoluteOffset) > GetSize())
			{
				m_outStream.seekp(0, std::ios::end);
				const auto extensionSize = absoluteOffset - static_cast<std::streamsize>(GetSize());
				const auto buffer = std::make_unique<char[]>(extensionSize);
				m_outStream.write(buffer.get(), extensionSize);
			}
			else
			{
				m_outStream.seekp(absoluteOffset);
			}
		}

		void Align(size_t alignment)
		{
			Seek(binaryio::Align(GetOffset(), alignment));
		}

		[[nodiscard]] size_t GetOffset()
		{
			return static_cast<size_t>(m_outStream.tellp());
		}

		[[nodiscard]] uint32_t GetOffset32()
		{
			const std::streamoff offset = m_outStream.tellp();

			if (!std::in_range<uint32_t>(offset))
				throw std::out_of_range("Offset out of 32-bit range");

			return static_cast<uint32_t>(offset);
		}

		[[nodiscard]] size_t GetSize()
		{
			m_outStream.seekg(0, std::ios::end);
			return m_outStream.tellg();
		}

		[[nodiscard]] std::stringstream GetStream()
		{
			m_outStream.seekg(0);
			return std::move(m_outStream);
		}

		[[nodiscard]] std::endian GetEndian() const noexcept
		{
			return m_endian;
		}

		void SetEndian(std::endian endian) noexcept
		{
			m_endian = endian;
		}

	private:
		std::stringstream m_outStream;
		std::queue<std::function<void(BinaryWriter &writer)>> m_deferredWrites;
		std::stack<std::queue<std::function<void(BinaryWriter &writer)>>> m_pushedDeferredWrites;
		std::endian m_endian{ std::endian::native };
	};
}
