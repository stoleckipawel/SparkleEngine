#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Files
{
	class BinarySpanReader final
	{
	  public:
		explicit BinarySpanReader(std::span<const std::uint8_t> bytes) noexcept : m_bytes(bytes) {}

		template <typename T> bool ReadValue(T& outValue, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinarySpanReader requires trivially-copyable values");

			if (!CanRead(sizeof(T)))
			{
				outErrorMessage = "Unexpected end of binary data";
				return false;
			}

			std::memcpy(&outValue, m_bytes.data() + m_offset, sizeof(T));
			m_offset += sizeof(T);
			return true;
		}

		template <typename T> bool ReadArray(std::size_t elementCount, std::vector<T>& outValues, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinarySpanReader requires trivially-copyable array elements");
			if (!TryGetArrayByteCount<T>(elementCount, outErrorMessage))
			{
				return false;
			}

			const std::size_t arrayByteCount = sizeof(T) * elementCount;
			if (!CanRead(arrayByteCount))
			{
				outErrorMessage = "Unexpected end of binary array data";
				return false;
			}

			outValues.resize(elementCount);
			if (arrayByteCount > 0)
			{
				std::memcpy(outValues.data(), m_bytes.data() + m_offset, arrayByteCount);
			}
			m_offset += arrayByteCount;
			return true;
		}

		template <typename T> bool ReadArrayView(std::size_t elementCount, std::span<const T>& outValues, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinarySpanReader requires trivially-copyable array elements");
			if (!TryGetArrayByteCount<T>(elementCount, outErrorMessage))
			{
				return false;
			}

			const std::size_t byteCount = sizeof(T) * elementCount;
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of binary array data";
				return false;
			}

			outValues = std::span<const T>(reinterpret_cast<const T*>(m_bytes.data() + m_offset), elementCount);
			m_offset += byteCount;
			return true;
		}

		template <typename T> bool SkipArray(std::size_t elementCount, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinarySpanReader requires trivially-copyable array elements");
			if (!TryGetArrayByteCount<T>(elementCount, outErrorMessage))
			{
				return false;
			}

			const std::size_t byteCount = sizeof(T) * elementCount;
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of binary array data";
				return false;
			}

			m_offset += byteCount;
			return true;
		}

		bool ReadBytes(std::size_t byteCount, std::span<const std::uint8_t>& outBytes, std::string& outErrorMessage)
		{
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of binary byte payload";
				return false;
			}

			outBytes = std::span<const std::uint8_t>(m_bytes.data() + m_offset, byteCount);
			m_offset += byteCount;
			return true;
		}

		bool ReadStringWithUInt32Length(std::string& outValue, std::string& outErrorMessage)
		{
			std::uint32_t sizeInBytes = 0;
			if (!ReadValue(sizeInBytes, outErrorMessage))
			{
				return false;
			}

			std::span<const std::uint8_t> bytes;
			if (!ReadBytes(sizeInBytes, bytes, outErrorMessage))
			{
				return false;
			}

			outValue.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
			return true;
		}

		std::size_t GetOffset() const noexcept { return m_offset; }
		std::size_t GetRemainingByteCount() const noexcept { return m_bytes.size() - m_offset; }

	  private:
		template <typename T> static bool TryGetArrayByteCount(std::size_t elementCount, std::string& outErrorMessage)
		{
			if constexpr (sizeof(T) > 0)
			{
				if (elementCount > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
				{
					outErrorMessage = "Binary array size overflow";
					return false;
				}
			}
			return true;
		}

		bool CanRead(std::size_t byteCount) const noexcept { return byteCount <= m_bytes.size() - m_offset; }

		std::span<const std::uint8_t> m_bytes;
		std::size_t m_offset = 0;
	};
}  // namespace Files