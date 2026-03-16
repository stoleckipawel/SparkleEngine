#include "PCH.h"
#include "Level/LevelParser.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Level/Level.h"

#include <cstdint>
#include <fstream>
#include <iomanip>

namespace
{
	enum class LevelFileSection : std::uint8_t
	{
		None = 0,
		Level,
		Camera,
		Meshes
	};

	struct ParsedLevelLine
	{
		std::string key;
		std::string value;
	};

	LevelFileSection ParseSection(std::string_view line)
	{
		const std::string sectionName = Engine::Strings::TrimCopy(line.substr(1, line.size() - 2));
		if (sectionName == "Level")
		{
			return LevelFileSection::Level;
		}
		if (sectionName == "Camera")
		{
			return LevelFileSection::Camera;
		}
		if (sectionName == "Meshes")
		{
			return LevelFileSection::Meshes;
		}

		return LevelFileSection::None;
	}

	bool TryParseKeyValueLine(std::string_view line, ParsedLevelLine& parsedLine)
	{
		const std::size_t separatorIndex = line.find('=');
		if (separatorIndex == std::string::npos)
		{
			return false;
		}

		parsedLine.key = Engine::Strings::TrimCopy(line.substr(0, separatorIndex));
		parsedLine.value = Engine::Strings::TrimCopy(line.substr(separatorIndex + 1));
		return true;
	}

	bool ParseLevelSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
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

	bool ParseCameraSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		if (parsedLine.key == "Position")
		{
			if (!Engine::Strings::TryParseFloat3(parsedLine.value, levelDesc.cameraDesc.position))
			{
				errorMessage = "Invalid camera position";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "YawRadians")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.yawRadians))
			{
				errorMessage = "Invalid camera yaw";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "PitchRadians")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.pitchRadians))
			{
				errorMessage = "Invalid camera pitch";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "FovYDegrees")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.fovYDegrees))
			{
				errorMessage = "Invalid camera FOV";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "MoveSpeed")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.moveSpeed))
			{
				errorMessage = "Invalid camera move speed";
				return false;
			}
		}

		return true;
	}

	bool ParseMeshesSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Asset")
		{
			levelDesc.importedMeshRequests.push_back({Engine::Strings::UnquoteCopy(parsedLine.value)});
		}

		return true;
	}

	bool ParseField(
	    LevelFileSection currentSection,
	    const ParsedLevelLine& parsedLine,
	    LevelDesc& levelDesc,
	    std::string& errorMessage)
	{
		switch (currentSection)
		{
			case LevelFileSection::Level:
				return ParseLevelSectionField(parsedLine, levelDesc);

			case LevelFileSection::Camera:
				return ParseCameraSectionField(parsedLine, levelDesc, errorMessage);

			case LevelFileSection::Meshes:
				return ParseMeshesSectionField(parsedLine, levelDesc);

			case LevelFileSection::None:
			default:
				return true;
		}
	}

	void WriteLevelSection(std::ofstream& output, const Level& level)
	{
		output << "[Level]\n";
		output << "Name = " << level.GetName() << "\n\n";
	}

	void WriteCameraSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << std::setprecision(9);
		output << "[Camera]\n";
		output << "Position = " << levelDesc.cameraDesc.position.x << ", " << levelDesc.cameraDesc.position.y << ", "
		       << levelDesc.cameraDesc.position.z << "\n";
		output << "YawRadians = " << levelDesc.cameraDesc.yawRadians << "\n";
		output << "PitchRadians = " << levelDesc.cameraDesc.pitchRadians << "\n";
		output << "FovYDegrees = " << levelDesc.cameraDesc.fovYDegrees << "\n";
		output << "MoveSpeed = " << levelDesc.cameraDesc.moveSpeed << "\n\n";
	}

	void WriteMeshesSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << "[Meshes]\n";
		for (const ImportedMeshRequest& request : levelDesc.importedMeshRequests)
		{
			output << "Asset = " << request.assetPath.generic_string() << "\n";
		}
	}
}

std::unique_ptr<Level> LevelParser::LoadFromFile(const std::filesystem::path& filePath, std::string& errorMessage)
{
	std::ifstream input(filePath);
	if (!input.is_open())
	{
		errorMessage = "Failed to open level file";
		return nullptr;
	}

	LevelFileSection currentSection = LevelFileSection::None;
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
			currentSection = ParseSection(line);
			continue;
		}

		ParsedLevelLine parsedLine;
		if (!TryParseKeyValueLine(line, parsedLine))
		{
			continue;
		}

		if (!ParseField(currentSection, parsedLine, levelDesc, errorMessage))
		{
			return nullptr;
		}
	}

	if (levelDesc.name.empty())
	{
		levelDesc.name = filePath.stem().string();
	}

	return std::make_unique<Level>(levelDesc, filePath);
}

bool LevelParser::SaveToFile(const Level& level, std::string* errorMessage)
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
	WriteCameraSection(output, levelDesc);
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