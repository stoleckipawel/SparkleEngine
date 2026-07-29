#pragma once

#include "Core/Public/Diagnostics/Error.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace Assets
{
	class CookedAssetByteReader final
	{
	  public:
		explicit CookedAssetByteReader(std::span<const std::uint8_t> bytes) noexcept : m_bytes(bytes) {}

		template <typename T> T Read()
		{
			static_assert(std::is_trivially_copyable_v<T>, "CookedAssetByteReader::Read accepts only trivially copyable types.");

			if (sizeof(T) > m_bytes.size() - m_offset)
			{
				throw Diagnostics::Error("Unexpected end of cooked asset data.");
			}

			T value;
			std::memcpy(&value, m_bytes.data() + m_offset, sizeof(T));
			m_offset += sizeof(T);
			return value;
		}

		template <typename T> std::vector<T> ReadArray(std::size_t elementCount)
		{
			static_assert(std::is_trivially_copyable_v<T>, "CookedAssetByteReader::ReadArray accepts only trivially copyable element types.");

			if (elementCount > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
			{
				throw Diagnostics::Error("Cooked asset array byte count exceeds the host address range.");
			}
			const std::size_t byteCount = sizeof(T) * elementCount;
			if (byteCount > m_bytes.size() - m_offset)
			{
				throw Diagnostics::Error("Unexpected end of cooked asset array data.");
			}

			std::vector<T> values(elementCount);
			if (byteCount > 0)
			{
				std::memcpy(values.data(), m_bytes.data() + m_offset, byteCount);
				m_offset += byteCount;
			}

			return values;
		}

		std::string ReadString(std::size_t byteCount)
		{
			if (byteCount > m_bytes.size() - m_offset)
			{
				throw Diagnostics::Error("Unexpected end of cooked asset string data.");
			}

			std::string value(reinterpret_cast<const char*>(m_bytes.data() + m_offset), byteCount);
			m_offset += byteCount;
			return value;
		}

		std::size_t GetRemainingByteCount() const noexcept { return m_bytes.size() - m_offset; }

	  private:
		std::span<const std::uint8_t> m_bytes;
		std::size_t m_offset = 0;
	};
}
