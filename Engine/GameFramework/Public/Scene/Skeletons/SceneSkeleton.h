#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct SPARKLE_ENGINE_API SceneJointDesc
{
	std::string name;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t parentJointIndex = (std::numeric_limits<std::uint32_t>::max)();
	DirectX::XMFLOAT4X4 inverseBindMatrix = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT4X4 bindPoseWorldTransform = MathUtils::IdentityFloat4x4();
};

struct SPARKLE_ENGINE_API SceneSkeletonDesc
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::uint32_t sourceSkinIndex = 0;
	std::vector<SceneJointDesc> joints;
};

struct SPARKLE_ENGINE_API SceneSkeletonPose
{
	Assets::CookedAssetId skeletonAssetId = Assets::InvalidCookedAssetId;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
};
