#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <iomanip>
#include <string>

namespace LevelParsing
{
	inline bool ParseCameraSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		if (parsedLine.key == "Position")
		{
			if (!Strings::TryParseFloat3(parsedLine.value, levelDesc.cameraDesc.position))
			{
				errorMessage = "Invalid camera position";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "YawRadians")
		{
			if (!Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.yawRadians))
			{
				errorMessage = "Invalid camera yaw";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "PitchRadians")
		{
			if (!Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.pitchRadians))
			{
				errorMessage = "Invalid camera pitch";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "FovYDegrees")
		{
			if (!Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.fovYDegrees))
			{
				errorMessage = "Invalid camera FOV";
				return false;
			}
			return true;
		}

		if (parsedLine.key == "MoveSpeed")
		{
			if (!Strings::TryParseFloat(parsedLine.value, levelDesc.cameraDesc.moveSpeed))
			{
				errorMessage = "Invalid camera move speed";
				return false;
			}
		}

		return true;
	}

	inline void WriteCameraSection(std::ofstream& output, const LevelDesc& levelDesc)
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
}  // namespace LevelParsing