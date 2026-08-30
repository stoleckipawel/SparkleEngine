#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace Json
{
	SPARKLE_CORE_API std::string QuoteString(std::string_view value);
	SPARKLE_CORE_API std::string QuoteHexUInt64(std::uint64_t value);

	class SPARKLE_CORE_API ObjectWriter final
	{
	public:
		explicit ObjectWriter(std::string indent = "  ");

		void WriteString(std::string_view name, std::string_view value);
		void WriteRaw(std::string_view name, std::string_view value);
		void WriteUInt64(std::string_view name, std::uint64_t value);
		void WriteHexUInt64(std::string_view name, std::uint64_t value);
		std::string Finish();

	private:
		void WritePrefix(std::string_view name);

		std::ostringstream m_stream;
		std::string m_indent;
		bool m_hasProperty = false;
	};

	SPARKLE_CORE_API std::string WriteStringArray(std::span<const std::string> values);
}
