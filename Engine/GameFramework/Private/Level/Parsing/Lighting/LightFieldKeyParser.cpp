#include "PCH.h"

#include "LightFieldKeyParser.h"

#include "Core/Public/Diagnostics/Error.h"

#include <array>
#include <cctype>
#include <format>
#include <limits>

namespace LevelParsing
{
	class LightFieldKeyParsing final
	{
	private:
		struct LightFieldPrefix final
		{
			std::string_view Text;
			SceneLightKind Kind = SceneLightKind::Unknown;
		};

		static constexpr std::array<LightFieldPrefix, 4> kPrefixes = {{
		    {"DirectionalLight", SceneLightKind::Directional},
		    {"PointLight", SceneLightKind::Point},
		    {"SpotLight", SceneLightKind::Spot},
		    {"RectLight", SceneLightKind::Rect},
		}};

		static ParsedLightFieldKey ParseIndexedField(std::string_view key, const LightFieldPrefix& prefix)
		{
			std::size_t cursor = prefix.Text.size();
			if (cursor >= key.size() || !std::isdigit(static_cast<unsigned char>(key[cursor])))
				throw Diagnostics::Error(std::format("Lighting field '{}' has no light index.", key));

			std::size_t index = 0;
			while (cursor < key.size() && std::isdigit(static_cast<unsigned char>(key[cursor])))
			{
				const std::size_t digit = static_cast<std::size_t>(key[cursor] - '0');
				if (index > ((std::numeric_limits<std::size_t>::max)() - digit) / 10u)
					throw Diagnostics::Error(std::format("Lighting field '{}' has an overflowing light index.", key));
				index = index * 10u + digit;
				++cursor;
			}
			if (cursor >= key.size())
				throw Diagnostics::Error(std::format("Lighting field '{}' has no property name.", key));
			return ParsedLightFieldKey{.Kind = prefix.Kind, .Index = index, .Field = key.substr(cursor)};
		}

	public:
		static ParsedLightFieldKey Parse(std::string_view key)
		{
			for (const LightFieldPrefix& prefix : kPrefixes)
			{
				if (key.starts_with(prefix.Text))
					return ParseIndexedField(key, prefix);
			}
			throw Diagnostics::Error(std::format("Unsupported lighting field '{}'.", key));
		}
	};

	ParsedLightFieldKey ParseLightFieldKey(std::string_view key)
	{
		return LightFieldKeyParsing::Parse(key);
	}
}
