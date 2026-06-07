#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API SceneAnimationKeyframe
{
	float timeSeconds = 0.0f;
	DirectX::XMFLOAT4 value = {};
	DirectX::XMFLOAT4 inTangent = {};
	DirectX::XMFLOAT4 outTangent = {};
};

struct SPARKLE_ENGINE_API SceneAnimationChannel
{
	Assets::CookedAnimationTargetPath targetPath = Assets::CookedAnimationTargetPath::Unknown;
	Assets::CookedAnimationInterpolation interpolation = Assets::CookedAnimationInterpolation::Linear;
	std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t targetJointIndex = Assets::kInvalidCookedAnimationJointIndex;
	std::uint32_t firstKeyframe = 0;
	std::uint32_t keyframeCount = 0;
};

struct SPARKLE_ENGINE_API SceneAnimationClipDesc
{
	Assets::CookedAssetId animationAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId targetSkeletonAssetId = Assets::InvalidCookedAssetId;
	std::string name;
	std::uint32_t sourceAnimationIndex = 0;
	float durationSeconds = 0.0f;
	std::uint32_t channelCount = 0;
	std::uint32_t keyframeCount = 0;
	std::vector<SceneAnimationChannel> channels;
	std::vector<SceneAnimationKeyframe> keyframes;
};

struct SPARKLE_ENGINE_API SceneAnimationPoseSnapshot
{
	Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId animationAssetId = Assets::InvalidCookedAssetId;
	std::string clipName;
	float playbackTimeSeconds = 0.0f;
	std::uint32_t jointCount = 0;
	std::vector<DirectX::XMFLOAT4X4> skinningMatrices;
};

struct SPARKLE_ENGINE_API SceneMorphWeightSnapshot
{
	std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::vector<float> weights;
};

struct SPARKLE_ENGINE_API SceneAnimationSnapshot
{
	std::vector<SceneAnimationPoseSnapshot> poses;
	std::vector<SceneMorphWeightSnapshot> morphWeights;

	void Reset() noexcept
	{
		poses.clear();
		morphWeights.clear();
	}
};
