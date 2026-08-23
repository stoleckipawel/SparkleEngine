#pragma once

#include "Gltf/GltfTangentFrameSetGenerator.h"

#include <cstdint>
#include <vector>

class GltfTangentVertexRemapper final
{
  public:
	static void Apply(ImportedMeshGeometry& geometry, const GltfGeneratedTangentFrameSet& frames);

  private:
	static constexpr float kTangentEqualityTolerance = 1.0e-6f;

	GltfTangentVertexRemapper(ImportedMeshGeometry& geometry, const GltfGeneratedTangentFrameSet& frames);

	static bool TangentsMatch(const DirectX::XMFLOAT4& first, const DirectX::XMFLOAT4& second) noexcept;
	static bool TangentsMatch(const DirectX::XMFLOAT3& first, const DirectX::XMFLOAT3& second) noexcept;
	bool FrameSetsMatch(std::uint32_t remappedVertexIndex, std::size_t cornerIndex) const noexcept;
	void Remap();
	std::uint32_t ResolveVertex(std::size_t cornerIndex);
	void Publish();

	ImportedMeshGeometry& m_geometry;
	const GltfGeneratedTangentFrameSet& m_frames;
	std::vector<std::vector<std::uint32_t>> m_variantsBySourceVertex;
	bool m_hasSkinInfluences = false;
	std::vector<ImportedVertex> m_vertices;
	std::vector<std::uint32_t> m_indices;
	std::vector<ImportedSkinInfluence> m_skinInfluences;
	std::vector<ImportedMorphTarget> m_morphTargets;
};
