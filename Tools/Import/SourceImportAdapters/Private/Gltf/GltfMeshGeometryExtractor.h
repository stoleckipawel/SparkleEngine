#pragma once

#include "Types/ImportedGeometry.h"

struct cgltf_primitive;

class GltfMeshGeometryExtractor final
{
  public:
	static ImportedMeshGeometry ExtractMeshGeometry(const cgltf_primitive& primitive);
};
