#pragma once

#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class GpuMesh;
struct MeshDraw;
struct PreparedRenderScene;
struct VertexSkinInfluence;

class RayTracingBlasGeometryBuilder final
{
public:
	static bool GeometryEquals(const RhiRayTracingGeometryDesc& left, const RhiRayTracingGeometryDesc& right) noexcept;
	static std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept;
	static bool IsSkinnedDraw(const MeshDraw& draw) noexcept;
	static void ComputeSkinnedPositions(
	    const PreparedRenderScene& preparedScene,
	    const MeshDraw& draw,
	    const GpuMesh& mesh,
	    std::vector<DirectX::XMFLOAT3>& outPositions) noexcept;

private:
	static DirectX::XMFLOAT3 TransformSkinnedPosition(
	    const DirectX::XMFLOAT3& position,
	    const VertexSkinInfluence& influence,
	    std::uint32_t jointMatrixOffset,
	    const std::vector<DirectX::XMFLOAT4X4>& jointMatrices) noexcept;
	static void ValidateMorphInputs(const PreparedRenderScene& preparedScene, const MeshDraw& draw, const GpuMesh& mesh) noexcept;
	static DirectX::XMFLOAT3 ApplyMorphPosition(
	    const DirectX::XMFLOAT3& position,
	    std::size_t vertexIndex,
	    const PreparedRenderScene& preparedScene,
	    const MeshDraw& draw,
	    const GpuMesh& mesh) noexcept;
};
