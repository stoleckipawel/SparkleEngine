#include "PCH.h"
#include "Level/Parsing/LevelParser.h"

#include "Level/Parsing/CameraSectionParser.h"
#include "Level/Parsing/LevelParserCommon.h"
#include "Level/Parsing/LightingSectionParser.h"
#include "Level/Parsing/SkySectionParser.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Level/Level.h"

#include <fstream>

class LevelParserOperations final
{
  public:
	static bool ParseLevelSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Name")
		{
			levelDesc.name = Strings::UnquoteCopy(parsedLine.value);
			return true;
		}

		if (parsedLine.key == "Description")
		{
			return true;
		}

		return true;
	}

	static bool ParseSceneAssetsSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Asset")
		{
			const std::string value = Strings::UnquoteCopy(parsedLine.value);
			const std::size_t separator = value.find('|');
			if (separator == std::string::npos)
				levelDesc.sceneAssetIds.push_back({value});
			else
				levelDesc.sceneAssetIds.push_back({
				    Strings::TrimCopy(std::string_view(value).substr(0, separator)),
				    Strings::TrimCopy(std::string_view(value).substr(separator + 1))});
		}

		return true;
	}

	static bool ParseField(
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

			case LevelParsing::LevelFileSection::Sky:
				return LevelParsing::ParseSkySectionField(parsedLine, levelDesc, errorMessage);

			case LevelParsing::LevelFileSection::Lighting:
				return LevelParsing::ParseLightingSectionField(parsedLine, levelDesc, errorMessage);

			case LevelParsing::LevelFileSection::SceneAssets:
				return ParseSceneAssetsSectionField(parsedLine, levelDesc);

			case LevelParsing::LevelFileSection::None:
			default:
				return true;
		}
	}

	static void WriteLevelSection(std::ofstream& output, const LevelAsset& level)
	{
		output << "[Level]\n";
		output << "Name = " << level.GetName() << "\n\n";
	}

	static void WriteSceneAssetsSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << "[SceneAssets]\n";
		for (const SceneAssetId& sceneAssetId : levelDesc.sceneAssetIds)
		{
			output << "Asset = " << sceneAssetId.value;
			if (!sceneAssetId.catalogValue.empty() && sceneAssetId.catalogValue != sceneAssetId.value)
				output << '|' << sceneAssetId.catalogValue;
			output << "\n";
		}
	}
};

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
		line = Strings::TrimCopy(line);
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
		if (!Strings::TrySplitKeyValue(line, '=', key, value))
		{
			continue;
		}
		parsedLine.key = std::string(key);
		parsedLine.value = std::string(value);

		if (!LevelParserOperations::ParseField(currentSection, parsedLine, levelDesc, errorMessage))
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
	LevelParserOperations::WriteLevelSection(output, level);
	LevelParsing::WriteCameraSection(output, levelDesc);
	LevelParsing::WriteSkySection(output, levelDesc);
	LevelParsing::WriteLightingSection(output, levelDesc);
	LevelParserOperations::WriteSceneAssetsSection(output, levelDesc);

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
