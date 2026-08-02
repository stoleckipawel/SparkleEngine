#include "PCH.h"

#include "Gltf/GltfMikkTangentContext.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <utility>

GltfMikkTangentContext::GltfMikkTangentContext(std::span<const ImportedVertex> vertices, std::span<const std::uint32_t> indices) :
    m_vertices(vertices),
    m_indices(indices)
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
			// A collapsed UV triangle has no source-defined tangent direction. MikkTSpace
			// deliberately leaves some such corners unset; use a stable orthogonal frame
			// there so the rest of an otherwise valid production mesh remains usable.
			m_cornerTangents[cornerIndex] = BuildFallbackTangent(m_vertices[m_indices[cornerIndex]].normal);
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
	return std::isfinite(tangent.x) && std::isfinite(tangent.y) && std::isfinite(tangent.z) && std::isfinite(tangent.w)
	    && tangentLengthSquared > kMinimumVectorLengthSquared && std::abs(tangentLengthSquared - 1.0f) <= kFrameTolerance
	    && std::abs(normalLengthSquared - 1.0f) <= kFrameTolerance && std::abs(tangentNormalDot) <= kFrameTolerance
	    && (tangent.w == -1.0f || tangent.w == 1.0f);
}

DirectX::XMFLOAT4 GltfMikkTangentContext::BuildFallbackTangent(const DirectX::XMFLOAT3& normal) noexcept
{
	const DirectX::XMVECTOR normalVector = DirectX::XMLoadFloat3(&normal);
	const DirectX::XMVECTOR reference =
	    std::abs(normal.z) < 0.999f ? DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) : DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT3 tangent;
	DirectX::XMStoreFloat3(&tangent, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(reference, normalVector)));
	return {tangent.x, tangent.y, tangent.z, 1.0f};
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
		// MikkTSpace defines stable fallback frames for isolated degenerate UVs and
		// triangles found in production glTF assets. Validate its emitted frames
		// after generation instead of rejecting those inputs preemptively.
		ValidateTriangle(faceIndex);
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
	if (!IsFinite(first.position) || !IsFinite(second.position) || !IsFinite(third.position) || !IsFinite(first.normal)
	    || !IsFinite(second.normal) || !IsFinite(third.normal) || !IsFinite(first.uv) || !IsFinite(second.uv) || !IsFinite(third.uv))
	{
		throw Diagnostics::Error(
		    std::format("Normal-mapped glTF triangle {} contains non-finite position, normal, or texture-coordinate data.", faceIndex));
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
