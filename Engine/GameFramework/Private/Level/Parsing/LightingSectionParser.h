#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <fstream>
namespace LevelParsing
{
	void ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc);
	void ValidateLightingSection(const LevelDesc& levelDesc);
	void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc);
}
