#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <iosfwd>
#include <string>

namespace LevelParsing
{
	bool ParseSkySectionField(
	    const ParsedLevelLine& parsedLine,
	    LevelDesc& levelDesc,
	    std::string& errorMessage);
	void WriteSkySection(std::ofstream& output, const LevelDesc& levelDesc);
}  // namespace LevelParsing
