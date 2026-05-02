#pragma once

#include "Core/Public/Strings/StringUtils.h"

#include <cstdint>
#include <cstdlib>
#include <string>

namespace Environment
{
	inline bool TryGetVariable(const char* name, std::string& outValue) noexcept
	{
		outValue.clear();
		if (name == nullptr)
		{
			return false;
		}

		char* rawValue = nullptr;
		size_t requiredLength = 0;
		if (_dupenv_s(&rawValue, &requiredLength, name) != 0 || rawValue == nullptr || requiredLength <= 1)
		{
			if (rawValue != nullptr)
			{
				std::free(rawValue);
			}
			return false;
		}

		outValue.assign(rawValue, requiredLength - 1);
		std::free(rawValue);
		return true;
	}

	inline bool GetFlag(const char* name) noexcept
	{
		std::string value;
		if (!TryGetVariable(name, value))
		{
			return false;
		}

		return !value.empty() && value[0] != '0';
	}

	inline std::uint32_t GetUInt32(const char* name, std::uint32_t fallbackValue) noexcept
	{
		std::string value;
		if (!TryGetVariable(name, value))
		{
			return fallbackValue;
		}

		std::uint32_t parsedValue = fallbackValue;
		return Strings::TryParseNumber(value, parsedValue) ? parsedValue : fallbackValue;
	}
}  // namespace Environment