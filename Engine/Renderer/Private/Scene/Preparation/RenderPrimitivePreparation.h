#pragma once

#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/RenderMeshWorldBounds.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <span>

struct ResolvedRenderPrimitive final
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

struct PreparedRenderPrimitive final
{
	RenderObjectId Object;
	MeshDraw Draw;
	RenderMeshWorldBounds WorldBounds;
	MaterialGpuHandle Material;
	RenderMeshInstanceGroupIndex InstanceGroupIndex = kInvalidRenderMeshInstanceGroupIndex;
	std::uint32_t MaterialAlphaMode = 0u;
};

class RenderPrimitivePreparation final
{
public:
	static void TransformRange(std::span<const ResolvedRenderPrimitive> inputs, std::span<PreparedRenderPrimitive> outputs) noexcept;

private:
	static RenderMeshWorldBounds TransformBounds(const MeshDrawGeometry& geometry, const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	static std::array<DirectX::XMFLOAT3, 8> BuildBoundsCorners(const MeshDrawGeometry& geometry) noexcept;
	static DirectX::XMFLOAT3 TransformPoint(const DirectX::XMFLOAT3& point, DirectX::FXMMATRIX worldMatrix) noexcept;
	static void ExpandBounds(const DirectX::XMFLOAT3& point, RenderMeshWorldBounds& bounds) noexcept;
};
