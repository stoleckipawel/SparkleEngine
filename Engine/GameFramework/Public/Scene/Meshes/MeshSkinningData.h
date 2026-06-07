#pragma once

#include "MeshData.h"

#include <cstdint>
#include <type_traits>
#include <vector>

struct VertexSkinInfluence
{
	std::uint16_t jointIndices[4] = {0, 0, 0, 0};
	float jointWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct SkeletalMeshData
{
	MeshData geometry;
	std::vector<VertexSkinInfluence> skinInfluences;

	bool IsValid() const noexcept { return geometry.IsValid() && skinInfluences.size() == geometry.vertices.size(); }
	std::uint32_t GetSkinInfluenceCount() const noexcept { return static_cast<std::uint32_t>(skinInfluences.size()); }
};

static_assert(std::is_trivially_copyable_v<VertexSkinInfluence>, "VertexSkinInfluence must be trivially copyable for GPU upload");
