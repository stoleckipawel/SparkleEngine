#include "PCH.h"
#include "Level/Parsing/LevelParser.h"

#include "Level/Parsing/CameraSectionParser.h"
#include "Level/Parsing/LevelParserCommon.h"
#include "Level/Parsing/LightingSectionParser.h"
#include "Level/Parsing/SkySectionParser.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Level/Level.h"

#include <fstream>
#include <format>

class LevelDocumentCodec final
{
public:
	static void ParseLevelSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Name")
		{
			levelDesc.name = Strings::UnquoteCopy(parsedLine.value);
			if (levelDesc.name.empty())
				throw Diagnostics::Error("Level name cannot be empty.");
			return;
		}

		throw Diagnostics::Error("Unsupported level field: " + parsedLine.key);
	}

	static void ParseSceneAssetsSectionField(const LevelParsing::ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key != "Asset")
			throw Diagnostics::Error("Unsupported scene-assets field: " + parsedLine.key);

		const std::string value = Strings::UnquoteCopy(parsedLine.value);
		const std::size_t separator = value.find('|');
		if (separator != std::string::npos && value.find('|', separator + 1u) != std::string::npos)
			throw Diagnostics::Error("Scene asset reference contains multiple catalog separators.");

		SceneAssetId assetId;
		assetId.value =
		    Strings::TrimCopy(separator == std::string::npos ? std::string_view(value) : std::string_view(value).substr(0, separator));
		if (separator != std::string::npos)
		{
			assetId.catalogValue = Strings::TrimCopy(std::string_view(value).substr(separator + 1u));
		}
		if (assetId.value.empty() || (separator != std::string::npos && assetId.catalogValue.empty()))
			throw Diagnostics::Error("Scene asset reference is empty.");
		levelDesc.sceneAssetIds.push_back(std::move(assetId));
	}

	static void ParseField(
	    LevelParsing::LevelFileSection currentSection,
	    const LevelParsing::ParsedLevelLine& parsedLine,
	    LevelDesc& levelDesc)
	{
		switch (currentSection)
		{
			case LevelParsing::LevelFileSection::Level:
				ParseLevelSectionField(parsedLine, levelDesc);
				return;

			case LevelParsing::LevelFileSection::Camera:
				LevelParsing::ParseCameraSectionField(parsedLine, levelDesc);
				return;

			case LevelParsing::LevelFileSection::Sky:
				LevelParsing::ParseSkySectionField(parsedLine, levelDesc);
				return;

			case LevelParsing::LevelFileSection::Lighting:
				LevelParsing::ParseLightingSectionField(parsedLine, levelDesc);
				return;

			case LevelParsing::LevelFileSection::SceneAssets:
				ParseSceneAssetsSectionField(parsedLine, levelDesc);
				return;

			case LevelParsing::LevelFileSection::None:
			default:
				throw Diagnostics::Error("Level field appears outside a supported section.");
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

std::unique_ptr<LevelAsset> LevelParser::LoadFromFile(const std::filesystem::path& filePath)
{
	std::ifstream input(filePath);
	if (!input.is_open())
		throw Diagnostics::Error(std::format("Failed to open level file '{}'.", filePath.string()));

	LevelParsing::LevelFileSection currentSection = LevelParsing::LevelFileSection::None;
	LevelDesc levelDesc;
	std::size_t lineNumber = 0;

	for (std::string line; std::getline(input, line);)
	{
		++lineNumber;
		line = Strings::TrimCopy(line);
		if (line.empty() || line[0] == '#' || line[0] == ';')
		{
			continue;
		}

		try
		{
			if (line.front() == '[' && line.back() == ']')
			{
				currentSection = LevelParsing::ParseSection(line);
				continue;
			}

			const LevelParsing::ParsedLevelLine parsedLine = LevelParsing::ParseField(line);
			LevelDocumentCodec::ParseField(currentSection, parsedLine, levelDesc);
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(std::format("{} ({}:{})", error.what(), filePath.string(), lineNumber));
		}
	}

	if (input.bad())
		throw Diagnostics::Error(std::format("Failed while reading level file '{}'.", filePath.string()));

	if (levelDesc.name.empty())
		throw Diagnostics::Error(std::format("Level file '{}' has no level name.", filePath.string()));
	LevelParsing::ValidateCameraSection(levelDesc);
	LevelParsing::ValidateLightingSection(levelDesc);

	return std::make_unique<LevelAsset>(levelDesc, filePath);
}

void LevelParser::SaveToFile(const LevelAsset& level)
{
	const std::filesystem::path& sourcePath = level.GetSourcePath();
	if (sourcePath.empty())
		throw Diagnostics::Error("Level has no source path.");

	std::error_code errorCode;
	std::filesystem::create_directories(sourcePath.parent_path(), errorCode);
	if (errorCode)
		throw Diagnostics::Error(std::format("Failed to create level directory '{}'.", sourcePath.parent_path().string()));

	std::ofstream output(sourcePath, std::ios::trunc);
	if (!output.is_open())
		throw Diagnostics::Error(std::format("Failed to open level file '{}' for writing.", sourcePath.string()));

	const LevelDesc levelDesc = level.BuildDescription();
	LevelDocumentCodec::WriteLevelSection(output, level);
	LevelParsing::WriteCameraSection(output, levelDesc);
	LevelParsing::WriteSkySection(output, levelDesc);
	LevelParsing::WriteLightingSection(output, levelDesc);
	LevelDocumentCodec::WriteSceneAssetsSection(output, levelDesc);

	if (!output.good())
		throw Diagnostics::Error(std::format("Failed while writing level file '{}'.", sourcePath.string()));
}
