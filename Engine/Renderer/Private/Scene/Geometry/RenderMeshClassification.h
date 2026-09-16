#pragma once

#include <cstdint>
#include <limits>

enum class RenderMeshKind : std::uint32_t
{
	Static = 0,
	Skeletal = 1,
};

enum class RenderMeshInstanceGroupKind : std::uint32_t
{
	None = 0,
	SharedMeshReference = 1,
	AuthoredInstanceGroup = 2,
};

using RenderMeshInstanceGroupIndex = std::uint32_t;

inline constexpr RenderMeshInstanceGroupIndex kInvalidRenderMeshInstanceGroupIndex =
    (std::numeric_limits<RenderMeshInstanceGroupIndex>::max)();

struct RenderMeshInstanceGroup final
{
	RenderMeshInstanceGroupKind groupKind = RenderMeshInstanceGroupKind::None;
	std::uint32_t instanceCount = 0;
};
