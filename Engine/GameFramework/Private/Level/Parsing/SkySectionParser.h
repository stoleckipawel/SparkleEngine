#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <iosfwd>
namespace LevelParsing
{
	void ParseSkySectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc);
	void WriteSkySection(std::ofstream& output, const LevelDesc& levelDesc);
}  // namespace LevelParsing
