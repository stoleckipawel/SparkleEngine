#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Files
{
	class SPARKLE_CORE_API BinaryBufferWriter final
	{
	  public:
		explicit BinaryBufferWriter(std::vector<std::uint8_t>& bytes) noexcept;

		template <typename T> void WriteValue(const T& value)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryBufferWriter requires trivially-copyable values");
			const auto* begin = reinterpret_cast<const std::uint8_t*>(&value);
			m_bytes.insert(m_bytes.end(), begin, begin + sizeof(T));
		}

		template <typename T> void WriteArray(std::span<const T> values)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryBufferWriter requires trivially-copyable array elements");
			if (values.empty())
			{
				return;
			}

			const auto* begin = reinterpret_cast<const std::uint8_t*>(values.data());
			m_bytes.insert(m_bytes.end(), begin, begin + sizeof(T) * values.size());
		}

		void WriteBytes(std::span<const std::uint8_t> values);
		bool WriteStringWithUInt32Length(std::string_view value, std::string& outErrorMessage);

	  private:
		std::vector<std::uint8_t>& m_bytes;
	};
}  // namespace Files
