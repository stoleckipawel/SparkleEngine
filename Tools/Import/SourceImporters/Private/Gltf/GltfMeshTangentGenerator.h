#pragma once

#include "Types/ImportedGeometry.h"

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

class GltfMeshTangentGenerator final
{
  public:
	static void GenerateTangents(ImportedMeshGeometry& geometry);

  private:
	static void AccumulateTriangle(
	    const ImportedMeshGeometry& geometry,
	    std::size_t firstIndex,
	    std::vector<DirectX::XMFLOAT3>& tangentSums,
	    std::vector<DirectX::XMFLOAT3>& bitangentSums);
	static void BuildVertexTangent(
	    ImportedVertex& vertex,
	    const DirectX::XMFLOAT3& tangentSum,
	    const DirectX::XMFLOAT3& bitangentSum);
};
