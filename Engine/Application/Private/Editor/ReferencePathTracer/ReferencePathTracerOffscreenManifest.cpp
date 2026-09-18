#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerOffscreenManifest.h"

#include "Core/Public/Json/JsonReader.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <system_error>
#include <utility>

bool ReferencePathTracerOffscreenManifest::Parse(std::string_view document, std::string& errorMessage)
{
	static constexpr std::array<std::string_view, 16> allowed = {
	    "schema",
	    "project",
	    "level",
	    "camera",
	    "product",
	    "width",
	    "height",
	    "filter",
	    "targetSpp",
	    "seed",
	    "replicate",
	    "backend",
	    "timeoutSeconds",
	    "maxOutputGiB",
	    "checkpointPolicy",
	    "outputDirectory"};

	m_values.clear();
	std::size_t cursor = 0u;
	Json::SkipWhitespace(document, cursor);
	if (cursor >= document.size() || document[cursor++] != '{')
	{
		errorMessage = "request-root-invalid";
		return false;
	}
	Json::SkipWhitespace(document, cursor);
	while (cursor < document.size() && document[cursor] != '}')
	{
		std::string key;
		if (!Json::TryReadString(document, cursor, key) || std::find(allowed.begin(), allowed.end(), key) == allowed.end()
		    || m_values.contains(key))
		{
			errorMessage = "request-member-unknown-or-duplicate";
			return false;
		}
		Json::SkipWhitespace(document, cursor);
		if (cursor >= document.size() || document[cursor++] != ':')
		{
			errorMessage = "request-member-invalid";
			return false;
		}
		Json::SkipWhitespace(document, cursor);

		Value value;
		if (cursor < document.size() && document[cursor] == '"')
		{
			value.Kind = ValueKind::String;
			if (!Json::TryReadString(document, cursor, value.String))
			{
				errorMessage = "request-string-invalid";
				return false;
			}
		}
		else
		{
			value.Kind = ValueKind::UnsignedInteger;
			const std::size_t numberStart = cursor;
			while (cursor < document.size() && std::isdigit(static_cast<unsigned char>(document[cursor])))
			{
				++cursor;
			}
			const std::from_chars_result parsed =
			    std::from_chars(document.data() + numberStart, document.data() + cursor, value.UnsignedInteger);
			if (cursor == numberStart || (document[numberStart] == '0' && cursor != numberStart + 1u) || parsed.ec != std::errc{}
			    || parsed.ptr != document.data() + cursor)
			{
				errorMessage = "request-number-invalid";
				return false;
			}
		}

		m_values.emplace(std::move(key), std::move(value));
		Json::SkipWhitespace(document, cursor);
		if (cursor >= document.size() || (document[cursor] != ',' && document[cursor] != '}'))
		{
			errorMessage = "request-member-invalid";
			return false;
		}
		if (document[cursor] == ',')
		{
			++cursor;
			Json::SkipWhitespace(document, cursor);
			if (cursor >= document.size() || document[cursor] == '}')
			{
				errorMessage = "request-member-invalid";
				return false;
			}
		}
		else
		{
			break;
		}
	}

	if (cursor >= document.size() || document[cursor++] != '}')
	{
		errorMessage = "request-root-invalid";
		return false;
	}
	Json::SkipWhitespace(document, cursor);
	if (cursor != document.size())
	{
		errorMessage = "request-trailing-content";
		return false;
	}
	return true;
}

bool ReferencePathTracerOffscreenManifest::Read(std::string_view key, std::string& value) const
{
	const auto found = m_values.find(std::string(key));
	if (found == m_values.end() || found->second.Kind != ValueKind::String)
	{
		return false;
	}
	value = found->second.String;
	return true;
}

bool ReferencePathTracerOffscreenManifest::Read(std::string_view key, std::uint64_t& value) const
{
	const auto found = m_values.find(std::string(key));
	if (found == m_values.end() || found->second.Kind != ValueKind::UnsignedInteger)
	{
		return false;
	}
	value = found->second.UnsignedInteger;
	return true;
}

bool ReferencePathTracerOffscreenManifest::ReadOptional(std::string_view key, std::string& value) const
{
	return !m_values.contains(std::string(key)) || Read(key, value);
}

bool ReferencePathTracerOffscreenManifest::ReadOptional(std::string_view key, std::uint64_t& value) const
{
	return !m_values.contains(std::string(key)) || Read(key, value);
}
