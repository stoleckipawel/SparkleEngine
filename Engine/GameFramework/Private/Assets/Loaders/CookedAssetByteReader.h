#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/BinarySpanReader.h"

#include <limits>

namespace Assets
{
	class CookedAssetByteReader final
	{
	public:
		explicit CookedAssetByteReader(std::span<const std::uint8_t> bytes) noexcept :
		    m_reader(bytes)
		{
		}

		template <typename T> T Read()
		{
			T value;
			std::string error;
			if (!m_reader.ReadValue(value, error))
			{
				throw Diagnostics::Error("Unexpected end of cooked asset data.");
			}
			return value;
		}

		template <typename T> std::vector<T> ReadArray(std::size_t elementCount)
		{
			std::vector<T> values;
			std::string error;
			if (!m_reader.ReadArray(elementCount, values, error))
			{
				ThrowArrayError<T>(elementCount);
			}
			return values;
		}

		template <typename T> std::span<const std::uint8_t> ReadArrayBytes(std::size_t elementCount)
		{
			std::span<const std::uint8_t> bytes;
			std::string error;
			if (!m_reader.ReadArrayBytes<T>(elementCount, bytes, error))
			{
				ThrowArrayError<T>(elementCount);
			}
			return bytes;
		}

		std::span<const std::uint8_t> ReadBytes(std::size_t byteCount)
		{
			std::span<const std::uint8_t> bytes;
			std::string error;
			if (!m_reader.ReadBytes(byteCount, bytes, error))
			{
				throw Diagnostics::Error("Unexpected end of cooked asset data.");
			}
			return bytes;
		}

		std::string ReadString(std::size_t byteCount)
		{
			std::string value;
			std::string error;
			if (!m_reader.ReadString(byteCount, value, error))
			{
				throw Diagnostics::Error("Unexpected end of cooked asset string data.");
			}
			return value;
		}

		std::size_t GetRemainingByteCount() const noexcept { return m_reader.GetRemainingByteCount(); }

	private:
		template <typename T> [[noreturn]] static void ThrowArrayError(std::size_t elementCount)
		{
			if (elementCount > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
			{
				throw Diagnostics::Error("Cooked asset array byte count exceeds the host address range.");
			}
			throw Diagnostics::Error("Unexpected end of cooked asset data.");
		}

		Files::BinarySpanReader m_reader;
	};
}
