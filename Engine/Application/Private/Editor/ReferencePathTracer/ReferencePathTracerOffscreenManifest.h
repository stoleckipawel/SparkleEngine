#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

class ReferencePathTracerOffscreenManifest final
{
public:
	bool Parse(std::string_view document, std::string& errorMessage);
	bool Read(std::string_view key, std::string& value) const;
	bool Read(std::string_view key, std::uint64_t& value) const;
	bool ReadOptional(std::string_view key, std::string& value) const;
	bool ReadOptional(std::string_view key, std::uint64_t& value) const;

private:
	enum class ValueKind : std::uint8_t
	{
		String,
		UnsignedInteger,
	};

	struct Value final
	{
		ValueKind Kind = ValueKind::String;
		std::string String;
		std::uint64_t UnsignedInteger = 0u;
	};

	std::unordered_map<std::string, Value> m_values;
};
