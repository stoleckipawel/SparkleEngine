#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "ImportedSceneIndices.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using ImportedSkeletonIndex = std::uint32_t;

inline constexpr ImportedSkeletonIndex kInvalidImportedSkeletonIndex = (std::numeric_limits<ImportedSkeletonIndex>::max)();

struct ImportedSkinInfluence
{
	std::uint16_t jointIndices[4] = {0, 0, 0, 0};
	float jointWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

static_assert(std::is_trivially_copyable_v<ImportedSkinInfluence>, "ImportedSkinInfluence must be trivially copyable for mesh cooking");

struct ImportedJoint
{
	std::string name;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t parentJointIndex = (std::numeric_limits<std::uint32_t>::max)();
	DirectX::XMFLOAT4X4 inverseBindMatrix = MathUtils::IdentityFloat4x4();
	DirectX::XMFLOAT4X4 bindPoseWorldTransform = MathUtils::IdentityFloat4x4();
};

struct ImportedSkeleton
{
	std::string name;
	std::uint32_t sourceSkinIndex = 0;
	std::uint32_t sourceSkeletonRootNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::vector<ImportedJoint> joints;

	bool IsValid() const noexcept { return !joints.empty(); }
};
