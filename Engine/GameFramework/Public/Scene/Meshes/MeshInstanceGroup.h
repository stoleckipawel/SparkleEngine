#pragma once

#include <cstdint>
#include <limits>
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"

using SceneMeshAssetIndex = std::uint32_t;
using SceneMeshInstanceIndex = std::uint32_t;
using SceneMeshInstanceGroupIndex = std::uint32_t;

inline constexpr SceneMeshAssetIndex kInvalidSceneMeshAssetIndex = (std::numeric_limits<SceneMeshAssetIndex>::max)();
inline constexpr SceneMeshInstanceIndex kInvalidSceneMeshInstanceIndex = (std::numeric_limits<SceneMeshInstanceIndex>::max)();
inline constexpr SceneMeshInstanceGroupIndex kInvalidSceneMeshInstanceGroupIndex =
    (std::numeric_limits<SceneMeshInstanceGroupIndex>::max)();

enum class SceneMeshInstanceGroupKind : std::uint32_t
{
	None = 0,
	SharedMeshReference = 1,
	AuthoredInstanceGroup = 2,
};

struct SceneMeshInstanceGroupData final
{
	Assets::CookedAssetId meshAssetId = Assets::InvalidCookedAssetId;
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	MaterialHandle materialHandle = MaterialHandle::Invalid();
	SceneMeshInstanceIndex firstInstance = kInvalidSceneMeshInstanceIndex;
	std::uint32_t instanceCount = 0;
	SceneMeshInstanceGroupKind groupKind = SceneMeshInstanceGroupKind::None;
	std::uint32_t flags = 0;
};
