#include "PCH.h"
#include "Level/Parsing/LevelParser.h"

#include "Level/Parsing/CameraSectionParser.h"
#include "Level/Parsing/LevelParserCommon.h"
#include "Level/Parsing/LightingSectionParser.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Level/Level.h"

#include <fstream>

namespace
{
	bool ParseLevelSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Name")
		{
			levelDesc.name = Engine::Strings::UnquoteCopy(parsedLine.value);
			return true;
		}

		if (parsedLine.key == "Description")
		{
			return true;
		}

		return true;
	}

	bool ParseMeshesSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Asset")
		{
			levelDesc.importedMeshRequests.push_back({Engine::Strings::UnquoteCopy(parsedLine.value)});
		}

		return true;
	}

	bool ParseField(
	    LevelParsing::LevelFileSection currentSection,
	    const LevelParsing::ParsedLevelLine& parsedLine,
	    LevelDesc& levelDesc,
	    std::string& errorMessage)
	{
		switch (currentSection)
		{
			case LevelParsing::LevelFileSection::Level:
				return ParseLevelSectionField(parsedLine, levelDesc);

			case LevelParsing::LevelFileSection::Camera:
				return LevelParsing::ParseCameraSectionField(parsedLine, levelDesc, errorMessage);

			case LevelParsing::LevelFileSection::Lighting:
				return LevelParsing::ParseLightingSectionField(parsedLine, levelDesc, errorMessage);

			case LevelParsing::LevelFileSection::Meshes:
				return ParseMeshesSectionField(parsedLine, levelDesc);

			case LevelParsing::LevelFileSection::None:
			default:
				return true;
		}
	}

	void WriteLevelSection(std::ofstream& output, const LevelAsset& level)
	{
		output << "[Level]\n";
		output << "Name = " << level.GetName() << "\n\n";
	}

	void WriteMeshesSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << "[Meshes]\n";
		for (const ImportedMeshRequest& request : levelDesc.importedMeshRequests)
		{
			output << "Asset = " << request.assetPath.generic_string() << "\n";
		}
	}
}  // namespace

std::unique_ptr<LevelAsset> LevelParser::LoadFromFile(const std::filesystem::path& filePath, std::string& errorMessage)
{
	std::ifstream input(filePath);
	if (!input.is_open())
	{
		errorMessage = "Failed to open level file";
		return nullptr;
	}

	LevelParsing::LevelFileSection currentSection = LevelParsing::LevelFileSection::None;
	LevelDesc levelDesc;

	for (std::string line; std::getline(input, line);)
	{
		line = Engine::Strings::TrimCopy(line);
		if (line.empty() || line[0] == '#' || line[0] == ';')
		{
			continue;
		}

		if (line.front() == '[' && line.back() == ']')
		{
			currentSection = LevelParsing::ParseSection(line);
			continue;
		}

		LevelParsing::ParsedLevelLine parsedLine;
		std::string_view key;
		std::string_view value;
		if (!Engine::Strings::TrySplitKeyValue(line, '=', key, value))
		{
			continue;
		}
		parsedLine.key = std::string(key);
		parsedLine.value = std::string(value);

		if (!ParseField(currentSection, parsedLine, levelDesc, errorMessage))
		{
			return nullptr;
		}
	}

	if (levelDesc.name.empty())
	{
		levelDesc.name = filePath.stem().string();
	}

	return std::make_unique<LevelAsset>(levelDesc, filePath);
}

bool LevelParser::SaveToFile(const LevelAsset& level, std::string* errorMessage)
{
	const std::filesystem::path& sourcePath = level.GetSourcePath();
	if (sourcePath.empty())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "Level has no source path";
		}
		return false;
	}

	std::error_code errorCode;
	std::filesystem::create_directories(sourcePath.parent_path(), errorCode);
	if (errorCode)
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "Failed to create level directory";
		}
		return false;
	}

	std::ofstream output(sourcePath, std::ios::trunc);
	if (!output.is_open())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "Failed to open level file for writing";
		}
		return false;
	}

	const LevelDesc levelDesc = level.BuildDescription();
	WriteLevelSection(output, level);
	LevelParsing::WriteCameraSection(output, levelDesc);
	LevelParsing::WriteLightingSection(output, levelDesc);
	WriteMeshesSection(output, levelDesc);

	if (!output.good())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "Failed while writing level file";
		}
		return false;
	}

	return true;
}