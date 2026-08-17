#pragma once

#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "SceneData/MaterialData.h"
#include "SceneData/MeshInstanceBatch.h"
#include "SceneData/RenderMeshWorldBounds.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <span>

struct Frustum;

struct ResolvedRenderObject final
{
	RenderObjectId Object;
	MeshDraw Draw;
	DirectX::XMFLOAT4X4 WorldMatrix = {};
	DirectX::XMFLOAT4X4 PreviousWorldMatrix = {};
	DirectX::XMFLOAT3X4 WorldInverseTranspose = {};
	MaterialGpuHandle Material;
	RenderMeshInstanceGroupIndex InstanceGroupIndex = kInvalidRenderMeshInstanceGroupIndex;
	std::uint32_t MaterialAlphaMode = 0u;
	std::uint32_t MorphTargetCount = 0u;
	std::uint32_t MorphTargetVertexCount = 0u;
};

struct PreparedRenderObject final
{
	RenderObjectId Object;
	MeshDraw Draw;
	RenderMeshWorldBounds WorldBounds;
	MaterialGpuHandle Material;
	RenderMeshInstanceGroupIndex InstanceGroupIndex = kInvalidRenderMeshInstanceGroupIndex;
	std::uint32_t MaterialAlphaMode = 0u;
	RenderMaterialClassification MaterialClassification = RenderMaterialClassification::Rejected;
	float CameraDistanceSquared = 0.0f;
	bool RasterVisible = false;
};

class RenderObjectPreparation final
{
public:
	static void TransformRange(std::span<const ResolvedRenderObject> inputs, std::span<PreparedRenderObject> outputs) noexcept;
	static void EvaluateVisibilityRange(
	    const Frustum& frustum,
	    const DirectX::XMFLOAT3& cameraPosition,
	    std::span<PreparedRenderObject> objects) noexcept;

private:
	static RenderMeshWorldBounds TransformBounds(const MeshDrawGeometry& geometry, const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	static std::array<DirectX::XMFLOAT3, 8> BuildBoundsCorners(const MeshDrawGeometry& geometry) noexcept;
	static DirectX::XMFLOAT3 TransformPoint(const DirectX::XMFLOAT3& point, DirectX::FXMMATRIX worldMatrix) noexcept;
	static void ExpandBounds(const DirectX::XMFLOAT3& point, RenderMeshWorldBounds& bounds) noexcept;
	static bool Intersects(const Frustum& frustum, const RenderMeshWorldBounds& bounds) noexcept;
	static RenderMaterialClassification ClassifyMaterial(std::uint32_t alphaMode) noexcept;
	static float ComputeCameraDistanceSquared(
	    const DirectX::XMFLOAT3& cameraPosition,
	    const RenderMeshWorldBounds& bounds,
	    const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
};
