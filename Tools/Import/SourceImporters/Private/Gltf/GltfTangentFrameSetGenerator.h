#pragma once

#include "Types/ImportedGeometry.h"

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

struct GltfGeneratedTangentFrameSet final
{
	std::vector<DirectX::XMFLOAT4> baseCornerTangents;
	std::vector<std::vector<DirectX::XMFLOAT3>> morphCornerTangentDeltas;
};

class GltfTangentFrameSetGenerator final
{
  public:
	static GltfGeneratedTangentFrameSet Generate(const ImportedMeshGeometry& geometry);

  private:
	static std::vector<ImportedVertex> BuildMorphVertices(const ImportedMeshGeometry& geometry, const ImportedMorphTarget& morphTarget);
	static std::vector<DirectX::XMFLOAT3> BuildMorphTangentDeltas(
	    const std::vector<DirectX::XMFLOAT4>& baseTangents,
	    const std::vector<DirectX::XMFLOAT4>& targetTangents,
	    std::size_t targetIndex);
};
