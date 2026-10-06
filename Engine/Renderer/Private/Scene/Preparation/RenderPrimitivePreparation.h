#pragma once

#include "Scene/Geometry/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/RenderMeshWorldBounds.h"

#include <DirectXMath.h>

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

void PrepareRenderPrimitives(std::span<const ResolvedRenderPrimitive> inputs, std::span<PreparedRenderPrimitive> outputs) noexcept;
