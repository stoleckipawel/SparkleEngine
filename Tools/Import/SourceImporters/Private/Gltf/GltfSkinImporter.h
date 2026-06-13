#pragma once

#include "SourceImportResult.h"

#include <cstddef>

struct cgltf_accessor;
struct cgltf_data;
struct cgltf_skin;

class GltfSkinImporter final
{
  public:
	static ImportedSkeletonIndex ImportSkeleton(const cgltf_data* data, const cgltf_skin* skin, SourceImportResult& result);
	static ImportedSkinInfluence ReadSkinInfluence(
	    const cgltf_accessor* joints,
	    const cgltf_accessor* weights,
	    std::size_t vertexIndex) noexcept;
};
