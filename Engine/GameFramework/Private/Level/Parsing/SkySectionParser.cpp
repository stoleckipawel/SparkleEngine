#include "PCH.h"

#include "Level/Parsing/SkySectionParser.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <iomanip>

namespace LevelParsing
{
	void ParseSkySectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (!levelDesc.sky)
		{
			levelDesc.sky.emplace();
		}

		SceneSkyDesc& sky = *levelDesc.sky;
		if (parsedLine.key == "Enabled")
		{
			sky.enabled = ParseBool(parsedLine.value, "sky enabled value");
		}
		else if (parsedLine.key == "Color")
		{
			sky.color = ParseFloat3(parsedLine.value, "sky color");
		}
		else if (parsedLine.key == "Brightness")
		{
			sky.brightness = ParseFloat(parsedLine.value, "sky brightness");
		}
		else if (parsedLine.key == "Texture")
		{
			sky.skyTexture.texturePath = Strings::UnquoteCopy(parsedLine.value);
			sky.skyTexture.textureGroup = TextureGroup::HdrColor;
		}
		else
		{
			throw Diagnostics::Error("Unsupported sky field: " + parsedLine.key);
		}
	}

	void WriteSkySection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		if (!levelDesc.sky)
		{
			return;
		}

		const SceneSkyDesc& sky = *levelDesc.sky;
		output << std::setprecision(9);
		output << "[Sky]\n";
		output << "Enabled = " << (sky.enabled ? "true" : "false") << "\n";
		output << "Color = " << sky.color.x << ", " << sky.color.y << ", " << sky.color.z << "\n";
		output << "Brightness = " << sky.brightness << "\n";
		output << "Texture = " << sky.skyTexture.texturePath << "\n\n";
	}
}  // namespace LevelParsing
