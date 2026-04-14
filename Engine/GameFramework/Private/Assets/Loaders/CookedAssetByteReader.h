#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace Engine::Assets
{
	class CookedAssetByteReader final
	{
	  public:
		explicit CookedAssetByteReader(std::span<const std::uint8_t> bytes) noexcept : m_bytes(bytes) {}

		template <typename T> bool Read(T& outValue, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "CookedAssetByteReader::Read requires trivially copyable types.");

			if (!CanRead(sizeof(T)))
			{
				outErrorMessage = "Unexpected end of cooked asset data";
				return false;
			}

			std::memcpy(&outValue, m_bytes.data() + m_offset, sizeof(T));
			m_offset += sizeof(T);
			return true;
		}

		template <typename T> bool ReadArray(std::size_t elementCount, std::vector<T>& outValues, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "CookedAssetByteReader::ReadArray requires trivially copyable element types.");

			const std::size_t byteCount = sizeof(T) * elementCount;
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of cooked asset array data";
				return false;
			}

			outValues.resize(elementCount);
			if (byteCount > 0)
			{
				std::memcpy(outValues.data(), m_bytes.data() + m_offset, byteCount);
				m_offset += byteCount;
			}

			return true;
		}

		bool ReadString(std::size_t byteCount, std::string& outValue, std::string& outErrorMessage)
		{
			if (!CanRead(byteCount))
			{
				outErrorMessage = "Unexpected end of cooked asset string data";
				return false;
			}

			outValue.assign(reinterpret_cast<const char*>(m_bytes.data() + m_offset), byteCount);
			m_offset += byteCount;
			return true;
		}

		std::size_t GetRemainingByteCount() const noexcept { return m_bytes.size() - m_offset; }

	  private:
		bool CanRead(std::size_t byteCount) const noexcept { return m_offset + byteCount <= m_bytes.size(); }

		std::span<const std::uint8_t> m_bytes;
		std::size_t m_offset = 0;
	};
}