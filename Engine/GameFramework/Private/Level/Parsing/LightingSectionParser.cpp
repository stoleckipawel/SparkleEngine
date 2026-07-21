#include "PCH.h"

#include "Level/Parsing/LightingSectionParser.h"

#include "Level/Parsing/Lighting/LightFieldKeyParser.h"
#include "Level/Parsing/Lighting/LightFieldParser.h"
#include "Level/Parsing/Lighting/LightingSectionWriter.h"

namespace LevelParsing
{
	bool ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		ParsedLightFieldKey key;
		if (!TryParseLightFieldKey(parsedLine.key, key)) return true;
		const bool parsed = ParseLightField(key, parsedLine, levelDesc.lights, errorMessage);
		if (!parsed && errorMessage.empty())
			errorMessage = "Unsupported " + std::string(GetLightKindName(key.Kind)) + " light field: " + std::string(key.Field);
		return parsed;
	}

	void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		WriteLightingSectionValues(output, levelDesc);
	}
}
