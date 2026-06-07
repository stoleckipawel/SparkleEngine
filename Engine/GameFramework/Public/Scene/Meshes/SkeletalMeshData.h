#pragma once

#include "MeshData.h"
#include "MeshMorphData.h"
#include "MeshSkinningData.h"

#include <cstdint>
#include <vector>

struct SkeletalMeshData
{
	MeshData geometry;
	MeshMorphData morphTargets;
	std::vector<VertexSkinInfluence> skinInfluences;

	bool IsValid() const noexcept { return geometry.IsValid() && skinInfluences.size() == geometry.vertices.size(); }
	std::uint32_t GetSkinInfluenceCount() const noexcept { return static_cast<std::uint32_t>(skinInfluences.size()); }
};
