#include "PCH.h"

#include "Level/Parsing/CameraSectionParser.h"

#include "Core/Public/Diagnostics/Error.h"

#include <fstream>
#include <iomanip>

namespace LevelParsing
{
	void ParseCameraSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		if (parsedLine.key == "Position")
		{
			levelDesc.cameraDesc.position = ParseFloat3(parsedLine.value, "camera position");
			return;
		}
		if (parsedLine.key == "YawRadians")
		{
			levelDesc.cameraDesc.yawRadians = ParseFloat(parsedLine.value, "camera yaw");
			return;
		}
		if (parsedLine.key == "PitchRadians")
		{
			levelDesc.cameraDesc.pitchRadians = ParseFloat(parsedLine.value, "camera pitch");
			return;
		}
		if (parsedLine.key == "FovYDegrees")
		{
			levelDesc.cameraDesc.fovYDegrees = ParseFloat(parsedLine.value, "camera FOV");
			if (levelDesc.cameraDesc.fovYDegrees <= 0.0f || levelDesc.cameraDesc.fovYDegrees >= 180.0f)
				throw Diagnostics::Error("Camera FOV must be between 0 and 180 degrees.");
			return;
		}
		if (parsedLine.key == "MoveSpeed")
		{
			levelDesc.cameraDesc.moveSpeed = ParseFloat(parsedLine.value, "camera move speed");
			if (levelDesc.cameraDesc.moveSpeed < 0.0f)
				throw Diagnostics::Error("Camera move speed cannot be negative.");
			return;
		}
		if (parsedLine.key == "NearZ")
		{
			levelDesc.cameraDesc.nearZ = ParseFloat(parsedLine.value, "camera near plane");
			if (levelDesc.cameraDesc.nearZ <= 0.0f)
				throw Diagnostics::Error("Camera near plane must be positive.");
			return;
		}
		if (parsedLine.key == "FarZ")
		{
			levelDesc.cameraDesc.farZ = ParseFloat(parsedLine.value, "camera far plane");
			if (levelDesc.cameraDesc.farZ <= 0.0f)
				throw Diagnostics::Error("Camera far plane must be positive.");
			return;
		}
		throw Diagnostics::Error("Unsupported camera field: " + parsedLine.key);
	}

	void ValidateCameraSection(const LevelDesc& levelDesc)
	{
		if (levelDesc.cameraDesc.farZ <= levelDesc.cameraDesc.nearZ)
			throw Diagnostics::Error("Camera far plane must be greater than its near plane.");
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
		output << "NearZ = " << levelDesc.cameraDesc.nearZ << "\n";
		output << "FarZ = " << levelDesc.cameraDesc.farZ << "\n";
		output << "MoveSpeed = " << levelDesc.cameraDesc.moveSpeed << "\n\n";
	}
}  // namespace LevelParsing
