#pragma once

#include "Types/ImportedGeometry.h"

struct cgltf_primitive;
struct cgltf_mesh;

class GltfMeshGeometryExtractor final
{
  public:
	static ImportedMeshGeometry ExtractMeshGeometry(const cgltf_mesh& mesh, const cgltf_primitive& primitive);
};
