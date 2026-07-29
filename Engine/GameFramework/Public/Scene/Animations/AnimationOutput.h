#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/World/EntityId.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API AnimationPoseOutput final
{
	EntityId animationEntity;
	Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId animationAssetId = Assets::InvalidCookedAssetId;
	std::string clipName;
	float playbackTimeSeconds = 0.0f;
	std::uint32_t jointCount = 0;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
};

struct SPARKLE_ENGINE_API MorphWeightOutput final
{
	EntityId animationEntity;
	std::uint32_t targetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::vector<float> weights;
};

struct SPARKLE_ENGINE_API AnimationOutput final
{
	std::vector<AnimationPoseOutput> poses;
	std::vector<MorphWeightOutput> morphWeights;

	void Reset() noexcept
	{
		poses.clear();
		morphWeights.clear();
	}
};
