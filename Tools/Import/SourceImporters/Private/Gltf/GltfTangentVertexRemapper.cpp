#include "PCH.h"

#include "Gltf/GltfTangentVertexRemapper.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

void GltfTangentVertexRemapper::Apply(ImportedMeshGeometry& geometry, const GltfGeneratedTangentFrameSet& frames)
{
	GltfTangentVertexRemapper remapper(geometry, frames);
	remapper.Remap();
	remapper.Publish();
}

GltfTangentVertexRemapper::GltfTangentVertexRemapper(ImportedMeshGeometry& geometry, const GltfGeneratedTangentFrameSet& frames) :
    m_geometry(geometry),
    m_frames(frames),
    m_variantsBySourceVertex(geometry.vertices.size()),
    m_hasSkinInfluences(geometry.HasSkinInfluences())
{
	if (frames.baseCornerTangents.size() != geometry.indices.size() ||
	    frames.morphCornerTangentDeltas.size() != geometry.deformation.morphTargets.size())
	{
		throw Diagnostics::Error("Generated glTF tangent frames do not match the geometry and deformation streams.");
	}
	if (m_hasSkinInfluences && geometry.deformation.skinInfluences.size() != geometry.vertices.size())
	{
		throw Diagnostics::Error("Cannot remap incomplete glTF skin influences across tangent seams.");
	}

	m_vertices.reserve(geometry.indices.size());
	m_indices.reserve(geometry.indices.size());
	if (m_hasSkinInfluences)
	{
		m_skinInfluences.reserve(geometry.indices.size());
	}
	m_morphTargets.reserve(geometry.deformation.morphTargets.size());
	for (std::size_t targetIndex = 0; targetIndex < geometry.deformation.morphTargets.size(); ++targetIndex)
	{
		const ImportedMorphTarget& sourceTarget = geometry.deformation.morphTargets[targetIndex];
		if (sourceTarget.deltas.size() != geometry.vertices.size())
		{
			throw Diagnostics::Error("Cannot remap incomplete glTF morph-target data across tangent seams.");
		}
		if (frames.morphCornerTangentDeltas[targetIndex].size() != geometry.indices.size())
		{
			throw Diagnostics::Error("Generated glTF morph-target tangents do not match the index stream.");
		}
		ImportedMorphTarget target;
		target.name = sourceTarget.name;
		target.defaultWeight = sourceTarget.defaultWeight;
		target.deltas.reserve(geometry.indices.size());
		m_morphTargets.push_back(std::move(target));
	}
}

bool GltfTangentVertexRemapper::TangentsMatch(const DirectX::XMFLOAT4& first, const DirectX::XMFLOAT4& second) noexcept
{
	return first.w == second.w && std::abs(first.x - second.x) <= kTangentEqualityTolerance &&
	       std::abs(first.y - second.y) <= kTangentEqualityTolerance && std::abs(first.z - second.z) <= kTangentEqualityTolerance;
}

bool GltfTangentVertexRemapper::TangentsMatch(const DirectX::XMFLOAT3& first, const DirectX::XMFLOAT3& second) noexcept
{
	return std::abs(first.x - second.x) <= kTangentEqualityTolerance && std::abs(first.y - second.y) <= kTangentEqualityTolerance &&
	       std::abs(first.z - second.z) <= kTangentEqualityTolerance;
}

bool GltfTangentVertexRemapper::FrameSetsMatch(std::uint32_t remappedVertexIndex, std::size_t cornerIndex) const noexcept
{
	if (!TangentsMatch(m_vertices[remappedVertexIndex].tangent, m_frames.baseCornerTangents[cornerIndex]))
	{
		return false;
	}
	for (std::size_t targetIndex = 0; targetIndex < m_morphTargets.size(); ++targetIndex)
	{
		if (!TangentsMatch(
		        m_morphTargets[targetIndex].deltas[remappedVertexIndex].tangent,
		        m_frames.morphCornerTangentDeltas[targetIndex][cornerIndex]))
		{
			return false;
		}
	}
	return true;
}

void GltfTangentVertexRemapper::Remap()
{
	for (std::size_t cornerIndex = 0; cornerIndex < m_geometry.indices.size(); ++cornerIndex)
	{
		m_indices.push_back(ResolveVertex(cornerIndex));
	}
}

std::uint32_t GltfTangentVertexRemapper::ResolveVertex(std::size_t cornerIndex)
{
	const std::uint32_t sourceVertexIndex = m_geometry.indices[cornerIndex];
	std::vector<std::uint32_t>& variants = m_variantsBySourceVertex[sourceVertexIndex];
	const auto matchingVariant = std::find_if(
	    variants.begin(),
	    variants.end(),
	    [this, cornerIndex](std::uint32_t remappedVertexIndex)
	    {
		    return FrameSetsMatch(remappedVertexIndex, cornerIndex);
	    });
	if (matchingVariant != variants.end())
	{
		return *matchingVariant;
	}

	if (m_vertices.size() >= static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		throw Diagnostics::Error("Tangent seam splitting exceeds the engine vertex-index range.");
	}
	const std::uint32_t remappedVertexIndex = static_cast<std::uint32_t>(m_vertices.size());
	ImportedVertex vertex = m_geometry.vertices[sourceVertexIndex];
	vertex.tangent = m_frames.baseCornerTangents[cornerIndex];
	m_vertices.push_back(vertex);
	if (m_hasSkinInfluences)
	{
		m_skinInfluences.push_back(m_geometry.deformation.skinInfluences[sourceVertexIndex]);
	}
	for (std::size_t targetIndex = 0; targetIndex < m_morphTargets.size(); ++targetIndex)
	{
		ImportedMorphTargetDelta delta = m_geometry.deformation.morphTargets[targetIndex].deltas[sourceVertexIndex];
		delta.tangent = m_frames.morphCornerTangentDeltas[targetIndex][cornerIndex];
		m_morphTargets[targetIndex].deltas.push_back(delta);
	}
	variants.push_back(remappedVertexIndex);
	return remappedVertexIndex;
}

void GltfTangentVertexRemapper::Publish()
{
	m_geometry.vertices = std::move(m_vertices);
	m_geometry.indices = std::move(m_indices);
	m_geometry.deformation.skinInfluences = std::move(m_skinInfluences);
	m_geometry.deformation.morphTargets = std::move(m_morphTargets);
}
