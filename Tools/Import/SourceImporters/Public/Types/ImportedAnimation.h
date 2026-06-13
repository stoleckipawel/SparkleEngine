#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

enum class ImportedAnimationInterpolation : std::uint32_t
{
	Linear = 0,
	Step = 1,
	CubicSpline = 2,
};

enum class ImportedAnimationTargetPath : std::uint32_t
{
	Translation = 0,
	Rotation = 1,
	Scale = 2,
	Weights = 3,
	Unknown = 0xFFFFFFFFu,
};

struct ImportedAnimationKeyframe
{
	float timeSeconds = 0.0f;
	DirectX::XMFLOAT4 value = {};
	DirectX::XMFLOAT4 inTangent = {};
	DirectX::XMFLOAT4 outTangent = {};
};

struct ImportedAnimationSampler
{
	ImportedAnimationInterpolation interpolation = ImportedAnimationInterpolation::Linear;
	std::vector<ImportedAnimationKeyframe> keyframes;

	bool IsValid() const noexcept { return !keyframes.empty(); }
};

struct ImportedAnimationChannel
{
	ImportedAnimationTargetPath targetPath = ImportedAnimationTargetPath::Unknown;
	std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t targetJointIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t samplerIndex = (std::numeric_limits<std::uint32_t>::max)();
};

struct ImportedAnimationClip
{
	std::string name;
	std::uint32_t sourceAnimationIndex = 0;
	std::uint32_t targetSkeletonIndex = (std::numeric_limits<std::uint32_t>::max)();
	float durationSeconds = 0.0f;
	std::vector<ImportedAnimationSampler> samplers;
	std::vector<ImportedAnimationChannel> channels;

	bool IsValid() const noexcept { return !samplers.empty() && !channels.empty() && durationSeconds >= 0.0f; }
};
