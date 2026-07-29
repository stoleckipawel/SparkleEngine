#pragma once

#include "LightFieldKeyParser.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <vector>

namespace LevelParsing
{
	void ParseLightField(
	    const ParsedLightFieldKey& key,
	    const ParsedLevelLine& parsedLine,
	    std::vector<SceneLightDesc>& lights);
}
