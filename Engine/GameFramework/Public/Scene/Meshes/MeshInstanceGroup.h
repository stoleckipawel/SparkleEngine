#pragma once

#include <cstdint>
#include <limits>

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