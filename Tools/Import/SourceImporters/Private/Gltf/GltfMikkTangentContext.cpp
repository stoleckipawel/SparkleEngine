#include "PCH.h"

#include "Gltf/GltfMikkTangentContext.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <utility>

GltfMikkTangentContext::GltfMikkTangentContext(std::span<const ImportedVertex> vertices, std::span<const std::uint32_t> indices) :
    m_vertices(vertices), m_indices(indices)
{
}

std::vector<DirectX::XMFLOAT4> GltfMikkTangentContext::Generate()
{
	ValidateSourceGeometry();
	const float invalid = std::numeric_limits<float>::quiet_NaN();
	m_cornerTangents.assign(m_indices.size(), {invalid, invalid, invalid, invalid});

	SMikkTSpaceInterface interface = BuildInterface();
	SMikkTSpaceContext context{.m_pInterface = &interface, .m_pUserData = this};
	if (genTangSpaceDefault(&context) == 0)
	{
		throw Diagnostics::Error("MikkTSpace could not generate tangents for the glTF primitive.");
	}
	for (std::size_t cornerIndex = 0; cornerIndex < m_cornerTangents.size(); ++cornerIndex)
	{
		if (!IsUsableTangent(m_cornerTangents[cornerIndex], m_vertices[m_indices[cornerIndex]].normal))
		{
			throw Diagnostics::Error(
			    std::format(
			        "MikkTSpace did not produce a complete tangent basis for glTF triangle {}, corner {}.",
			        cornerIndex / kVerticesPerTriangle,
			        cornerIndex % kVerticesPerTriangle));
		}
	}

	return std::move(m_cornerTangents);
}

bool GltfMikkTangentContext::IsFinite(const DirectX::XMFLOAT2& value) noexcept
{
	return std::isfinite(value.x) && std::isfinite(value.y);
}

bool GltfMikkTangentContext::IsFinite(const DirectX::XMFLOAT3& value) noexcept
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool GltfMikkTangentContext::IsUsableTangent(const DirectX::XMFLOAT4& tangent, const DirectX::XMFLOAT3& normal) noexcept
{
	const float tangentLengthSquared = tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z;
	const float normalLengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
	const float tangentNormalDot = tangent.x * normal.x + tangent.y * normal.y + tangent.z * normal.z;
	return std::isfinite(tangent.x) && std::isfinite(tangent.y) && std::isfinite(tangent.z) && std::isfinite(tangent.w) &&
	       tangentLengthSquared > kMinimumVectorLengthSquared && std::abs(tangentLengthSquared - 1.0f) <= kFrameTolerance &&
	       std::abs(normalLengthSquared - 1.0f) <= kFrameTolerance && std::abs(tangentNormalDot) <= kFrameTolerance &&
	       (tangent.w == -1.0f || tangent.w == 1.0f);
}
void GltfMikkTangentContext::ValidateSourceGeometry() const
{
	if (m_vertices.empty() || m_indices.empty() || m_indices.size() % kVerticesPerTriangle != 0u)
	{
		throw Diagnostics::Error("Cannot generate a tangent basis for incomplete triangle geometry.");
	}
	if (m_indices.size() / kVerticesPerTriangle > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
	{
		throw Diagnostics::Error("The glTF primitive contains too many triangles for MikkTSpace.");
	}

	for (std::size_t faceIndex = 0; faceIndex < m_indices.size() / kVerticesPerTriangle; ++faceIndex)
	{
		ValidateTriangle(faceIndex);
		ValidateTangentDerivatives(faceIndex);
	}
}

void GltfMikkTangentContext::ValidateTriangle(std::size_t faceIndex) const
{
	const std::size_t firstCorner = faceIndex * kVerticesPerTriangle;
	const std::uint32_t firstIndex = m_indices[firstCorner];
	const std::uint32_t secondIndex = m_indices[firstCorner + 1u];
	const std::uint32_t thirdIndex = m_indices[firstCorner + 2u];
	if (firstIndex >= m_vertices.size() || secondIndex >= m_vertices.size() || thirdIndex >= m_vertices.size())
	{
		throw Diagnostics::Error(std::format("glTF triangle {} has an out-of-range vertex index.", faceIndex));
	}
	const ImportedVertex& first = m_vertices[firstIndex];
	const ImportedVertex& second = m_vertices[secondIndex];
	const ImportedVertex& third = m_vertices[thirdIndex];
	if (!IsFinite(first.position) || !IsFinite(second.position) || !IsFinite(third.position) || !IsFinite(first.normal) ||
	    !IsFinite(second.normal) || !IsFinite(third.normal) || !IsFinite(first.uv) || !IsFinite(second.uv) || !IsFinite(third.uv))
	{
		throw Diagnostics::Error(
		    std::format("Normal-mapped glTF triangle {} contains non-finite position, normal, or texture-coordinate data.", faceIndex));
	}
}

void GltfMikkTangentContext::ValidateTangentDerivatives(std::size_t faceIndex) const
{
	const std::size_t firstCorner = faceIndex * kVerticesPerTriangle;
	const ImportedVertex& first = m_vertices[m_indices[firstCorner]];
	const ImportedVertex& second = m_vertices[m_indices[firstCorner + 1u]];
	const ImportedVertex& third = m_vertices[m_indices[firstCorner + 2u]];
	const double firstEdgeX = static_cast<double>(second.position.x) - first.position.x;
	const double firstEdgeY = static_cast<double>(second.position.y) - first.position.y;
	const double firstEdgeZ = static_cast<double>(second.position.z) - first.position.z;
	const double secondEdgeX = static_cast<double>(third.position.x) - first.position.x;
	const double secondEdgeY = static_cast<double>(third.position.y) - first.position.y;
	const double secondEdgeZ = static_cast<double>(third.position.z) - first.position.z;
	const double crossX = firstEdgeY * secondEdgeZ - firstEdgeZ * secondEdgeY;
	const double crossY = firstEdgeZ * secondEdgeX - firstEdgeX * secondEdgeZ;
	const double crossZ = firstEdgeX * secondEdgeY - firstEdgeY * secondEdgeX;
	const double areaSquared = crossX * crossX + crossY * crossY + crossZ * crossZ;
	const double firstEdgeLengthSquared = firstEdgeX * firstEdgeX + firstEdgeY * firstEdgeY + firstEdgeZ * firstEdgeZ;
	const double secondEdgeLengthSquared = secondEdgeX * secondEdgeX + secondEdgeY * secondEdgeY + secondEdgeZ * secondEdgeZ;
	const double geometryScale = firstEdgeLengthSquared * secondEdgeLengthSquared;
	const double relativeTolerance = static_cast<double>(std::numeric_limits<float>::epsilon());
	if (geometryScale == 0.0 || areaSquared <= relativeTolerance * relativeTolerance * geometryScale)
	{
		throw Diagnostics::Error(
		    std::format("Normal-mapped glTF triangle {} has degenerate geometry and cannot define a tangent basis.", faceIndex));
	}

	const double firstUvEdgeX = static_cast<double>(second.uv.x) - first.uv.x;
	const double firstUvEdgeY = static_cast<double>(second.uv.y) - first.uv.y;
	const double secondUvEdgeX = static_cast<double>(third.uv.x) - first.uv.x;
	const double secondUvEdgeY = static_cast<double>(third.uv.y) - first.uv.y;
	const double determinant = firstUvEdgeX * secondUvEdgeY - firstUvEdgeY * secondUvEdgeX;
	const double determinantScale = std::abs(firstUvEdgeX * secondUvEdgeY) + std::abs(firstUvEdgeY * secondUvEdgeX);
	if (determinantScale == 0.0 || std::abs(determinant) <= relativeTolerance * determinantScale)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Normal-mapped glTF triangle {} has a singular texture-coordinate mapping and cannot define a tangent basis; "
		        "repair the source UVs or provide authored tangents.",
		        faceIndex));
	}
}

const ImportedVertex& GltfMikkTangentContext::GetVertex(int faceIndex, int faceVertexIndex) const noexcept
{
	const std::size_t cornerIndex = static_cast<std::size_t>(faceIndex) * kVerticesPerTriangle + static_cast<std::size_t>(faceVertexIndex);
	return m_vertices[m_indices[cornerIndex]];
}

GltfMikkTangentContext& GltfMikkTangentContext::GetOwner(const SMikkTSpaceContext* context) noexcept
{
	return *static_cast<GltfMikkTangentContext*>(context->m_pUserData);
}

int GltfMikkTangentContext::GetFaceCount(const SMikkTSpaceContext* context)
{
	return static_cast<int>(GetOwner(context).m_indices.size() / kVerticesPerTriangle);
}

int GltfMikkTangentContext::GetFaceVertexCount(const SMikkTSpaceContext*, int)
{
	return static_cast<int>(kVerticesPerTriangle);
}

void GltfMikkTangentContext::GetPosition(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex)
{
	const DirectX::XMFLOAT3& value = GetOwner(context).GetVertex(faceIndex, faceVertexIndex).position;
	output[0] = value.x;
	output[1] = value.y;
	output[2] = value.z;
}

void GltfMikkTangentContext::GetNormal(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex)
{
	const DirectX::XMFLOAT3& value = GetOwner(context).GetVertex(faceIndex, faceVertexIndex).normal;
	output[0] = value.x;
	output[1] = value.y;
	output[2] = value.z;
}

void GltfMikkTangentContext::GetTextureCoordinate(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex)
{
	const DirectX::XMFLOAT2& value = GetOwner(context).GetVertex(faceIndex, faceVertexIndex).uv;
	output[0] = value.x;
	output[1] = value.y;
}

void GltfMikkTangentContext::SetTangent(
    const SMikkTSpaceContext* context,
    const float tangent[],
    float sign,
    int faceIndex,
    int faceVertexIndex)
{
	GltfMikkTangentContext& owner = GetOwner(context);
	const std::size_t cornerIndex = static_cast<std::size_t>(faceIndex) * kVerticesPerTriangle + static_cast<std::size_t>(faceVertexIndex);
	owner.m_cornerTangents[cornerIndex] = {tangent[0], tangent[1], tangent[2], sign < 0.0f ? -1.0f : 1.0f};
}

SMikkTSpaceInterface GltfMikkTangentContext::BuildInterface() noexcept
{
	return SMikkTSpaceInterface{
	    .m_getNumFaces = &GetFaceCount,
	    .m_getNumVerticesOfFace = &GetFaceVertexCount,
	    .m_getPosition = &GetPosition,
	    .m_getNormal = &GetNormal,
	    .m_getTexCoord = &GetTextureCoordinate,
	    .m_setTSpaceBasic = &SetTangent,
	    .m_setTSpace = nullptr};
}
