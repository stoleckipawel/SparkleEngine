#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

struct SkeletonJoint final
{
	std::string name;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t parentJointIndex = (std::numeric_limits<std::uint32_t>::max)();
	DirectX::XMFLOAT4X4 inverseBindMatrix = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT4X4 bindLocalTransform = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT4X4 parentSpaceTransform = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT4X4 bindModelTransform = MathUtils::IdentityFloat4x4();
};

struct SkeletonResource final
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::uint32_t sourceSkinIndex = 0;
	std::vector<SkeletonJoint> joints;
};
