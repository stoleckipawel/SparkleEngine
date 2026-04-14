#include "PCH.h"

#include "Core/Public/Strings/StringUtils.h"

#include <cctype>

namespace Engine
{
	namespace Strings
	{
		std::string ToLowerCopy(std::string_view str)
		{
			std::string lowered(str);
			std::transform(
			    lowered.begin(),
			    lowered.end(),
			    lowered.begin(),
			    [](unsigned char ch)
			    {
				    return static_cast<char>(std::tolower(ch));
			    });
			return lowered;
		}

		bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept
		{
			if (lhs.size() != rhs.size())
			{
				return false;
			}

			for (std::size_t index = 0; index < lhs.size(); ++index)
			{
				const unsigned char lhsChar = static_cast<unsigned char>(lhs[index]);
				const unsigned char rhsChar = static_cast<unsigned char>(rhs[index]);
				if (std::tolower(lhsChar) != std::tolower(rhsChar))
				{
					return false;
				}
			}

			return true;
		}

		bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) noexcept
		{
			if (needle.empty())
			{
				return true;
			}

			const std::string loweredHaystack = ToLowerCopy(haystack);
			const std::string loweredNeedle = ToLowerCopy(needle);
			return loweredHaystack.find(loweredNeedle) != std::string::npos;
		}

		bool TryParseBool(std::string_view str, bool& outValue)
		{
			const std::string normalized = ToLowerCopy(TrimAsciiWhitespace(str));
			if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on")
			{
				outValue = true;
				return true;
			}

			if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off")
			{
				outValue = false;
				return true;
			}

			return false;
		}

		bool TrySplitKeyValue(std::string_view str, char separator, std::string_view& outKey, std::string_view& outValue) noexcept
		{
			const std::size_t separatorIndex = str.find(separator);
			if (separatorIndex == std::string::npos)
			{
				return false;
			}

			outKey = TrimAsciiWhitespace(str.substr(0, separatorIndex));
			outValue = TrimAsciiWhitespace(str.substr(separatorIndex + 1));
			return true;
		}
	}  // namespace Strings
}  // namespace Engine