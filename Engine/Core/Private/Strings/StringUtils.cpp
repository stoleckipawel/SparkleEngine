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
	}  // namespace Strings
}  // namespace Engine