#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <iosfwd>
#include <string>

namespace LevelParsing
{
	bool ParseCameraSectionField(
	    const ParsedLevelLine& parsedLine,
	    LevelDesc& levelDesc,
	    std::string& errorMessage);
	void WriteCameraSection(std::ofstream& output, const LevelDesc& levelDesc);
}  // namespace LevelParsing
