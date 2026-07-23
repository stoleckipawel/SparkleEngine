#include "PCH.h"

#include "Core/Public/Json/JsonWriter.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Strings/StringUtils.h"

#include <utility>

namespace Json
{
	std::string QuoteString(std::string_view value)
	{
		return "\"" + Strings::EscapeJsonString(value) + "\"";
	}

	std::string QuoteHexUInt64(std::uint64_t value)
	{
		return QuoteString(Formatting::FormatHexUInt64(value));
	}

	ObjectWriter::ObjectWriter(std::string indent) : m_indent(std::move(indent))
	{
		m_stream << "{\n";
	}

	void ObjectWriter::WriteString(std::string_view name, std::string_view value)
	{
		WritePrefix(name);
		m_stream << QuoteString(value);
	}

	void ObjectWriter::WriteRaw(std::string_view name, std::string_view value)
	{
		WritePrefix(name);
		m_stream << value;
	}

	void ObjectWriter::WriteUInt64(std::string_view name, std::uint64_t value)
	{
		WritePrefix(name);
		m_stream << value;
	}

	void ObjectWriter::WriteHexUInt64(std::string_view name, std::uint64_t value)
	{
		WritePrefix(name);
		m_stream << QuoteHexUInt64(value);
	}

	std::string ObjectWriter::Finish()
	{
		m_stream << "\n}\n";
		return m_stream.str();
	}

	void ObjectWriter::WritePrefix(std::string_view name)
	{
		if (m_hasProperty)
		{
			m_stream << ",\n";
		}
		m_stream << m_indent << QuoteString(name) << ": ";
		m_hasProperty = true;
	}

	std::string WriteStringArray(std::span<const std::string> values)
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
