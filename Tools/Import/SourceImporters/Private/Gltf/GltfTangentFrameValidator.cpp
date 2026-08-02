#include "PCH.h"

#include "Gltf/GltfTangentFrameValidator.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>
#include <format>

void GltfTangentFrameValidator::Validate(const ImportedMeshGeometry& geometry)
{
	if (geometry.vertices.empty() || geometry.indices.empty() || geometry.indices.size() % 3u != 0u)
	{
		throw Diagnostics::Error("Normal-mapped glTF geometry must contain complete indexed triangles.");
	}
	for (const std::uint32_t vertexIndex : geometry.indices)
	{
		if (vertexIndex >= geometry.vertices.size())
		{
			throw Diagnostics::Error("Normal-mapped glTF geometry contains an out-of-range triangle index.");
		}
	}

	for (std::size_t vertexIndex = 0; vertexIndex < geometry.vertices.size(); ++vertexIndex)
	{
		ValidateBaseFrame(geometry.vertices[vertexIndex], vertexIndex);
	}

	for (std::size_t targetIndex = 0; targetIndex < geometry.deformation.morphTargets.size(); ++targetIndex)
	{
		const ImportedMorphTarget& target = geometry.deformation.morphTargets[targetIndex];
		if (target.deltas.size() != geometry.vertices.size())
		{
			throw Diagnostics::Error(
			    std::format(
			        "glTF morph target {} contains {} deltas for {} remapped vertices.",
			        targetIndex,
			        target.deltas.size(),
			        geometry.vertices.size()));
		}
		for (std::size_t vertexIndex = 0; vertexIndex < geometry.vertices.size(); ++vertexIndex)
		{
			ValidateMorphFrame(geometry.vertices[vertexIndex], target.deltas[vertexIndex], targetIndex, vertexIndex);
		}
	}
}

bool GltfTangentFrameValidator::IsFinite(const DirectX::XMFLOAT3& value) noexcept
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

void GltfTangentFrameValidator::ValidateBaseFrame(const ImportedVertex& vertex, std::size_t vertexIndex)
{
	const DirectX::XMFLOAT3 tangent{vertex.tangent.x, vertex.tangent.y, vertex.tangent.z};
	if (!std::isfinite(vertex.tangent.w) || (vertex.tangent.w != -1.0f && vertex.tangent.w != 1.0f))
	{
		throw Diagnostics::Error(std::format("Normal-mapped glTF vertex {} has invalid tangent handedness.", vertexIndex));
	}
	ValidateDirections(vertex.normal, tangent, std::format("Normal-mapped glTF vertex {}", vertexIndex));

	const float normalLengthSquared =
	    vertex.normal.x * vertex.normal.x + vertex.normal.y * vertex.normal.y + vertex.normal.z * vertex.normal.z;
	const float tangentLengthSquared = tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z;
	const float tangentNormalDot = vertex.normal.x * tangent.x + vertex.normal.y * tangent.y + vertex.normal.z * tangent.z;
	if (std::abs(normalLengthSquared - 1.0f) > kUnitFrameTolerance || std::abs(tangentLengthSquared - 1.0f) > kUnitFrameTolerance
	    || std::abs(tangentNormalDot) > kUnitFrameTolerance)
	{
		throw Diagnostics::Error(std::format("Normal-mapped glTF vertex {} does not contain an orthonormal tangent frame.", vertexIndex));
	}
}

void GltfTangentFrameValidator::ValidateMorphFrame(
    const ImportedVertex& vertex,
    const ImportedMorphTargetDelta& delta,
    std::size_t targetIndex,
    std::size_t vertexIndex)
{
	const DirectX::XMFLOAT3 normal{vertex.normal.x + delta.normal.x, vertex.normal.y + delta.normal.y, vertex.normal.z + delta.normal.z};
	const DirectX::XMFLOAT3 tangent{
	    vertex.tangent.x + delta.tangent.x,
	    vertex.tangent.y + delta.tangent.y,
	    vertex.tangent.z + delta.tangent.z};
	ValidateDirections(normal, tangent, std::format("Normal-mapped glTF morph target {}, vertex {}", targetIndex, vertexIndex));
}

void GltfTangentFrameValidator::ValidateDirections(
    const DirectX::XMFLOAT3& normal,
    const DirectX::XMFLOAT3& tangent,
    std::string_view frameLabel)
{
	if (!IsFinite(normal) || !IsFinite(tangent))
	{
		throw Diagnostics::Error(std::format("{} contains non-finite normal or tangent data.", frameLabel));
	}
	const float normalLengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
	const float tangentLengthSquared = tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z;
	if (normalLengthSquared <= kMinimumDirectionLengthSquared || tangentLengthSquared <= kMinimumDirectionLengthSquared)
	{
		throw Diagnostics::Error(std::format("{} contains a zero-length normal or tangent.", frameLabel));
	}
	const float tangentNormalDot = normal.x * tangent.x + normal.y * tangent.y + normal.z * tangent.z;
	const float cosineSquared = tangentNormalDot * tangentNormalDot / (normalLengthSquared * tangentLengthSquared);
	if (cosineSquared >= 1.0f - kParallelTolerance)
	{
		throw Diagnostics::Error(std::format("{} contains parallel normal and tangent directions.", frameLabel));
	}
}
