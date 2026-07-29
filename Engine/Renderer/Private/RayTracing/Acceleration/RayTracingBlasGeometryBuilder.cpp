#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingBlasGeometryBuilder.h"

#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GpuMesh.h"
#include "SceneData/RenderSceneData.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <span>

static const auto g_rayTracingBlasGeometryBuilderLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.BlasGeometryBuilder");

bool RayTracingBlasGeometryBuilder::GeometryEquals(const RhiRayTracingGeometryDesc& left, const RhiRayTracingGeometryDesc& right) noexcept
{
	return left.VertexBuffer.Resource.Value == right.VertexBuffer.Resource.Value &&
	       left.VertexBuffer.OffsetInBytes == right.VertexBuffer.OffsetInBytes && left.VertexStrideInBytes == right.VertexStrideInBytes &&
	       left.VertexCount == right.VertexCount && left.IndexBuffer.Resource.Value == right.IndexBuffer.Resource.Value &&
	       left.IndexBuffer.OffsetInBytes == right.IndexBuffer.OffsetInBytes && left.IndexCount == right.IndexCount &&
	       left.IndexFormat == right.IndexFormat && left.Opaque == right.Opaque;
}

std::uint64_t RayTracingBlasGeometryBuilder::AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
{
	return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
}

bool RayTracingBlasGeometryBuilder::IsSkinnedDraw(const MeshDraw& draw) noexcept
{
	return draw.Geometry.MeshKind == RenderMeshKind::Skeletal;
}

void RayTracingBlasGeometryBuilder::ComputeSkinnedPositions(
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    const GpuMesh& mesh,
    std::vector<DirectX::XMFLOAT3>& outPositions) noexcept
{
	if (!mesh.HasRayTracingHitData() || !mesh.HasSkinInfluences() ||
	    mesh.GetRayTracingHitVertices().size() != mesh.GetSkinInfluences().size() || sceneData.jointMatrices.empty())
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasGeometryBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Skinned BLAS input has incomplete hit, skin-influence, or joint-matrix data.");
	}

	const std::span<const RayTracingHitVertex> vertices = mesh.GetRayTracingHitVertices();
	const std::span<const VertexSkinInfluence> skinInfluences = mesh.GetSkinInfluences();
	ValidateMorphInputs(sceneData, draw, mesh);

	outPositions.clear();
	outPositions.reserve(vertices.size());
	for (std::size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		for (std::uint32_t influenceIndex = 0u; influenceIndex < 8u; ++influenceIndex)
		{
			if (skinInfluences[vertexIndex].jointWeights[influenceIndex] <= 0.0f)
			{
				continue;
			}

			const std::uint32_t jointMatrixIndex =
			    draw.Skinning.JointMatrixOffset + skinInfluences[vertexIndex].jointIndices[influenceIndex];
			if (jointMatrixIndex >= sceneData.jointMatrices.size())
			{
				Diagnostics::Fatal(
				    g_rayTracingBlasGeometryBuilderLogger,
				    __FILE__,
				    __LINE__,
				    "Skinned BLAS vertex references a joint matrix outside the render scene.");
			}
		}

		outPositions.push_back(TransformSkinnedPosition(
		    ApplyMorphPosition(vertices[vertexIndex].Position, vertexIndex, sceneData, draw, mesh),
		    skinInfluences[vertexIndex],
		    draw.Skinning.JointMatrixOffset,
		    sceneData.jointMatrices));
	}

	if (outPositions.empty())
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasGeometryBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Skinned BLAS input contains no vertices.");
	}
}

DirectX::XMFLOAT3 RayTracingBlasGeometryBuilder::TransformSkinnedPosition(
    const DirectX::XMFLOAT3& position,
    const VertexSkinInfluence& influence,
    std::uint32_t jointMatrixOffset,
    const std::vector<DirectX::XMFLOAT4X4>& jointMatrices) noexcept
{
	const DirectX::XMVECTOR sourcePosition = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR skinnedPosition = DirectX::XMVectorZero();
	float totalWeight = 0.0f;

	for (std::uint32_t influenceIndex = 0u; influenceIndex < 8u; ++influenceIndex)
	{
		const float weight = influence.jointWeights[influenceIndex];
		if (weight <= 0.0f)
		{
			continue;
		}

		const std::uint32_t jointMatrixIndex = jointMatrixOffset + influence.jointIndices[influenceIndex];
		const DirectX::XMMATRIX skinningMatrix = DirectX::XMLoadFloat4x4(&jointMatrices[jointMatrixIndex]);
		skinnedPosition = DirectX::XMVectorAdd(
		    skinnedPosition,
		    DirectX::XMVectorScale(DirectX::XMVector3Transform(sourcePosition, skinningMatrix), weight));
		totalWeight += weight;
	}

	if (totalWeight <= 0.0f)
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasGeometryBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Skinned BLAS vertex has no positive joint influence.");
	}

	DirectX::XMFLOAT3 result;
	DirectX::XMStoreFloat3(&result, skinnedPosition);
	return result;
}

void RayTracingBlasGeometryBuilder::ValidateMorphInputs(
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    const GpuMesh& mesh) noexcept
{
	if (draw.Morph.TargetCount == 0u)
	{
		return;
	}

	if (draw.Morph.VertexCount != mesh.GetVertexCount() || mesh.GetMorphTargetCount() != draw.Morph.TargetCount ||
	    draw.Morph.WeightOffset > sceneData.morphWeights.size() ||
	    draw.Morph.TargetCount > sceneData.morphWeights.size() - draw.Morph.WeightOffset ||
	    mesh.GetMorphTargetDeltas().size() != static_cast<std::size_t>(draw.Morph.TargetCount) * draw.Morph.VertexCount)
	{
		Diagnostics::Fatal(
		    g_rayTracingBlasGeometryBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Skinned BLAS morph inputs do not match the mesh and render-scene ranges.");
	}
}

DirectX::XMFLOAT3 RayTracingBlasGeometryBuilder::ApplyMorphPosition(
    const DirectX::XMFLOAT3& position,
    std::size_t vertexIndex,
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    const GpuMesh& mesh) noexcept
{
	DirectX::XMFLOAT3 morphed = position;
	if (draw.Morph.TargetCount == 0u)
	{
		return morphed;
	}

	const std::span<const MorphTargetDeltaData> deltas = mesh.GetMorphTargetDeltas();
	for (std::uint32_t targetIndex = 0u; targetIndex < draw.Morph.TargetCount; ++targetIndex)
	{
		const float weight = sceneData.morphWeights[draw.Morph.WeightOffset + targetIndex];
		const MorphTargetDeltaData& delta = deltas[static_cast<std::size_t>(targetIndex) * draw.Morph.VertexCount + vertexIndex];

		morphed.x += delta.Position.x * weight;
		morphed.y += delta.Position.y * weight;
		morphed.z += delta.Position.z * weight;
	}

	return morphed;
}
