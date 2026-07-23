#include "PCH.h"

#include "Level/Parsing/LevelParserCommon.h"

#include "Core/Public/Strings/StringUtils.h"

namespace LevelParsing
{
	LevelFileSection ParseSection(std::string_view line)
	{
		const std::string sectionName = Strings::TrimCopy(line.substr(1, line.size() - 2));
		if (sectionName == "Level") return LevelFileSection::Level;
		if (sectionName == "Camera") return LevelFileSection::Camera;
		if (sectionName == "Sky") return LevelFileSection::Sky;
		if (sectionName == "Lighting") return LevelFileSection::Lighting;
		if (sectionName == "SceneAssets") return LevelFileSection::SceneAssets;
		return LevelFileSection::None;
	}
}  // namespace LevelParsing
