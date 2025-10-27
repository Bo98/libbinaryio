#pragma once
#include <cassert>
#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdint.h>
#include "util.hpp"

namespace binaryio
{
	class BinaryWriter
	{
	public:
		template<typename T>
		std::enable_if_t<std::is_arithmetic_v<typename SafeUnderlyingType<T>::type>> Write(T value)
		{
			if (m_bigEndian)
			{
				union {
					T val;
					uint8_t bytes[sizeof(T)];
				} processedValue;

				processedValue.val = value;

				for (auto i = 0U; i < sizeof(T) / 2; i++)
				{
					const auto tmp = processedValue.bytes[sizeof(T) - i - 1];
					processedValue.bytes[sizeof(T) - i - 1] = processedValue.bytes[i];
					processedValue.bytes[i] = tmp;
				}

				value = processedValue.val;
			}

			m_outStream.write(reinterpret_cast<const char *>(&value), sizeof(T));

			assert(!m_outStream.fail());
		}

		template<typename T>
		std::enable_if_t<HasValueType<T>::value && !HasColType<T>::value> Write(T value)
		{
			for (auto i = 0U; i < sizeof(T) / sizeof(typename T::value_type); i++)
				Write(value[i]);
		}

		template<typename T>
		std::enable_if_t<HasColType<T>::value> Write(T value)
		{
			for (auto i = 0U; i < sizeof(T) / sizeof(typename T::col_type); i++)
				Write(value[i]);
		}

		void Write(const std::string &value, bool nullTerminate = true)
		{
			if (value.empty())
			{
				if (nullTerminate)
					Write<uint8_t>(0);
				return;
			}

			m_outStream.write(value.c_str(), value.length() + size_t(nullTerminate ? 1 : 0));

			assert(!m_outStream.fail());
		}

		template<typename T>
		std::enable_if_t<std::is_pointer_v<T>> Write(T value, size_t elementCount)
		{
			for (auto i = 0U; i < elementCount; i++)
				Write(value[i]);
		}

		template<typename T>
		std::enable_if_t<std::is_arithmetic_v<typename SafeUnderlyingType<T>::type> || std::is_constructible_v<T, const std::string &>> VisitAndWrite(size_t &offset, T value)
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

			assert(!m_outStream.fail());
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
				absoluteOffset += GetOffset();
			else if (seekdir == std::ios::end)
				absoluteOffset += GetSize();

			if (absoluteOffset > GetSize())
			{
				m_outStream.seekp(0, std::ios::end);
				const auto extensionSize = absoluteOffset - GetSize();
				const auto buffer = new char[extensionSize]();
				m_outStream.write(buffer, extensionSize);
				delete[] buffer;
			}
			else
			{
				m_outStream.seekp(absoluteOffset);
			}

			assert(!m_outStream.fail());
		}

		void Align(size_t alignment)
		{
			Seek(binaryio::Align(GetOffset(), alignment));
		}

		size_t GetOffset()
		{
			return static_cast<size_t>(m_outStream.tellp());
		}

		size_t GetSize()
		{
			m_outStream.seekg(0, std::ios::end);
			return m_outStream.tellg();
		}

		std::stringstream GetStream()
		{
			m_outStream.seekg(0);
			return std::move(m_outStream);
		}

		bool IsBigEndian() const
		{
			return m_bigEndian;
		}

		void SetBigEndian(bool bigEndian)
		{
			m_bigEndian = bigEndian;
		}

	private:
		std::stringstream m_outStream;
		std::queue<std::function<void(BinaryWriter &writer)>> m_deferredWrites;
		std::stack<std::queue<std::function<void(BinaryWriter &writer)>>> m_pushedDeferredWrites;
		bool m_bigEndian = false;
	};
}
