#pragma once

#include "LightFieldKeyParser.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <string>
#include <vector>

namespace LevelParsing
{
	bool ParseLightField(
	    const ParsedLightFieldKey& key,
	    const ParsedLevelLine& parsedLine,
	    std::vector<SceneLightDesc>& lights,
	    std::string& errorMessage);
}
