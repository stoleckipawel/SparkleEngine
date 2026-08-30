#pragma once

#include "Types/ImportedMorphTarget.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct cgltf_mesh;
struct cgltf_primitive;

class GltfMorphTargetImporter final
{
public:
	static std::vector<ImportedMorphTarget> ImportMorphTargets(
	    const cgltf_mesh& mesh,
	    const cgltf_primitive& primitive,
	    std::uint32_t vertexCount);
	static std::vector<float> BuildNodeMorphWeights(
	    const cgltf_mesh& mesh,
	    const float* nodeWeights,
	    std::size_t nodeWeightCount,
	    std::size_t targetCount);
};
