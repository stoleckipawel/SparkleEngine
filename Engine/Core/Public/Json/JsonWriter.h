#pragma once

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"

#include <cstdint>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace Json
{
	inline std::string QuoteString(std::string_view value)
	{
		return "\"" + Strings::EscapeJsonString(value) + "\"";
	}

	inline std::string QuoteHexUInt64(std::uint64_t value)
	{
		return QuoteString(Formatting::FormatHexUInt64(value));
	}

	class ObjectWriter final
	{
	  public:
		explicit ObjectWriter(std::string indent = "  ") : m_indent(std::move(indent))
		{
			m_stream << "{\n";
		}

		void WriteString(std::string_view name, std::string_view value)
		{
			WritePrefix(name);
			m_stream << QuoteString(value);
		}

		void WriteRaw(std::string_view name, std::string_view value)
		{
			WritePrefix(name);
			m_stream << value;
		}

		void WriteUInt64(std::string_view name, std::uint64_t value)
		{
			WritePrefix(name);
			m_stream << value;
		}

		void WriteHexUInt64(std::string_view name, std::uint64_t value)
		{
			WritePrefix(name);
			m_stream << QuoteHexUInt64(value);
		}

		std::string Finish()
		{
			m_stream << "\n}\n";
			return m_stream.str();
		}

	  private:
		void WritePrefix(std::string_view name)
		{
			if (m_hasProperty)
			{
				m_stream << ",\n";
			}
			m_stream << m_indent << QuoteString(name) << ": ";
			m_hasProperty = true;
		}

		std::ostringstream m_stream;
		std::string m_indent;
		bool m_hasProperty = false;
	};

	inline std::string WriteStringArray(std::span<const std::string> values)
	{
		std::ostringstream stream;
		stream << "[\n";
		for (std::size_t index = 0; index < values.size(); ++index)
		{
			stream << "  " << QuoteString(values[index]);
			if (index + 1 < values.size())
			{
				stream << ',';
			}
			stream << "\n";
		}
		stream << "]\n";
		return stream.str();
	}
}  // namespace Json