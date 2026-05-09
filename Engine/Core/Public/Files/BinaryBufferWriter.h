#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Files
{
	class BinaryBufferWriter final
	{
	  public:
		explicit BinaryBufferWriter(std::vector<std::uint8_t>& bytes) noexcept : m_bytes(bytes) {}

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

		void WriteBytes(std::span<const std::uint8_t> values) { m_bytes.insert(m_bytes.end(), values.begin(), values.end()); }

		bool WriteStringWithUInt32Length(std::string_view value, std::string& outErrorMessage)
		{
			if (value.size() > static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
			{
				outErrorMessage = "Binary serialization failed: string field too large";
				return false;
			}

			const std::uint32_t sizeInBytes = static_cast<std::uint32_t>(value.size());
			WriteValue(sizeInBytes);
			const auto* begin = reinterpret_cast<const std::uint8_t*>(value.data());
			m_bytes.insert(m_bytes.end(), begin, begin + value.size());
			return true;
		}

	  private:
		std::vector<std::uint8_t>& m_bytes;
	};
}  // namespace Files