#pragma once

#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/MeshMorphData.h"

#include <span>

namespace MeshMorphEvaluator
{
	void ApplyWeights(MeshData& meshData, const MeshMorphData& morphTargets, std::span<const float> weights) noexcept;
}
