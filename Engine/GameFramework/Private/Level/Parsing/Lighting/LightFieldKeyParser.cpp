#include "PCH.h"

#include "LightFieldKeyParser.h"

#include <array>
#include <cctype>

namespace LevelParsing
{
	namespace
	{
		struct LightFieldPrefix final
		{
			std::string_view Text;
			SceneLightKind Kind = SceneLightKind::Unknown;
		};

		constexpr std::array<LightFieldPrefix, 4> kLightFieldPrefixes = {{
		    {"DirectionalLight", SceneLightKind::Directional},
		    {"PointLight", SceneLightKind::Point},
		    {"SpotLight", SceneLightKind::Spot},
		    {"RectLight", SceneLightKind::Rect},
		}};

		bool TryParseIndexedField(
		    std::string_view key,
		    std::string_view prefix,
		    std::size_t& outIndex,
		    std::string_view& outField) noexcept
		{
			if (!key.starts_with(prefix)) return false;
			std::size_t cursor = prefix.size();
			if (cursor >= key.size() || !std::isdigit(static_cast<unsigned char>(key[cursor]))) return false;

			std::size_t index = 0;
			while (cursor < key.size() && std::isdigit(static_cast<unsigned char>(key[cursor])))
			{
				index = (index * 10) + static_cast<std::size_t>(key[cursor] - '0');
				++cursor;
			}
			if (cursor >= key.size()) return false;
			outIndex = index;
			outField = key.substr(cursor);
			return true;
		}
	}

	bool TryParseLightFieldKey(std::string_view key, ParsedLightFieldKey& outKey) noexcept
	{
		for (const LightFieldPrefix& prefix : kLightFieldPrefixes)
		{
			std::size_t index = 0;
			std::string_view field;
			if (!TryParseIndexedField(key, prefix.Text, index, field)) continue;
			outKey = {.Kind = prefix.Kind, .Index = index, .Field = field};
			return true;
		}
		return false;
	}

	std::string_view GetLightKindName(SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional: return "directional";
			case SceneLightKind::Point: return "point";
			case SceneLightKind::Spot: return "spot";
			case SceneLightKind::Rect: return "rect";
			case SceneLightKind::Unknown:
			default: return "unknown";
		}
	}
}
