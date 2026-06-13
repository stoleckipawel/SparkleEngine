#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <fstream>
#include <string>

namespace LevelParsing
{
	bool ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage);
	void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc);
}  // namespace LevelParsing
