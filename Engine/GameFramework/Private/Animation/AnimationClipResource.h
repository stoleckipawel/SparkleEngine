#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct AnimationKeyframe final
{
	float timeSeconds = 0.0f;
	DirectX::XMFLOAT4 value = {};
	DirectX::XMFLOAT4 inTangent = {};
	DirectX::XMFLOAT4 outTangent = {};
};

struct AnimationChannel final
{
	Assets::CookedAnimationTargetPath targetPath = Assets::CookedAnimationTargetPath::Unknown;
	Assets::CookedAnimationInterpolation interpolation = Assets::CookedAnimationInterpolation::Linear;
	std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t targetJointIndex = Assets::kInvalidCookedAnimationJointIndex;
	std::uint32_t firstKeyframe = 0;
	std::uint32_t keyframeCount = 0;
};

struct AnimationClipResource final
{
	Assets::CookedAssetId animationAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId targetSkeletonAssetId = Assets::InvalidCookedAssetId;
	std::string name;
	std::uint32_t sourceAnimationIndex = 0;
	float durationSeconds = 0.0f;
	std::uint32_t channelCount = 0;
	std::uint32_t keyframeCount = 0;
	std::vector<AnimationChannel> channels;
	std::vector<AnimationKeyframe> keyframes;
};
