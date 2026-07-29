#pragma once

#include "Types/ImportedGeometry.h"

#include <cstdint>

struct cgltf_primitive;
struct cgltf_mesh;

class GltfMeshGeometryExtractor final
{
  public:
	static ImportedMeshGeometry ExtractMeshGeometry(const cgltf_mesh& mesh, const cgltf_primitive& primitive);

  private:
	struct Attributes;

	static Attributes CollectAttributes(const cgltf_primitive& primitive) noexcept;
	static void ValidateAttributes(const cgltf_primitive& primitive, const Attributes& attributes);
	static void PopulateVertices(const Attributes& attributes, std::uint32_t vertexCount, ImportedMeshGeometry& geometry);
	static void PopulateIndices(const cgltf_primitive& primitive, std::uint32_t vertexCount, ImportedMeshGeometry& geometry);
};
