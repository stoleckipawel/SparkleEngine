#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include <iosfwd>
namespace LevelParsing
{
	void ParseCameraSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc);
	void ValidateCameraSection(const LevelDesc& levelDesc);
	void WriteCameraSection(std::ofstream& output, const LevelDesc& levelDesc);
}
