#include "PCH.h"

#include "Core/Public/Files/BinaryBufferWriter.h"

#include <limits>

namespace Files
{
	BinaryBufferWriter::BinaryBufferWriter(std::vector<std::uint8_t>& bytes) noexcept :
	    m_bytes(bytes)
	{
	}

	void BinaryBufferWriter::WriteBytes(std::span<const std::uint8_t> values)
	{
		m_bytes.insert(m_bytes.end(), values.begin(), values.end());
	}

	bool BinaryBufferWriter::WriteStringWithUInt32Length(std::string_view value, std::string& outErrorMessage)
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
}
