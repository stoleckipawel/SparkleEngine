#pragma once

#include "../RendererAPI.h"
#include "Renderer/Public/SceneData/RenderMeshClassification.h"

#include <DirectXMath.h>
#include <cstdint>
#include <limits>

class GPUMesh;

enum class MeshInstanceBatchSource : std::uint32_t
{
	PreservedGroup = 0,
	AuthoredGroup = 1,
	AutoBatch = 2,
	SingleInstance = 3,
};

struct SPARKLE_RENDERER_API MeshDrawTransform final
{
	DirectX::XMFLOAT4X4 WorldMatrix = {};
	DirectX::XMFLOAT4X4 PreviousWorldMatrix = {};
	DirectX::XMFLOAT3X4 WorldInvTranspose = {};
};

struct SPARKLE_RENDERER_API MeshDrawMaterial final
{
	std::uint32_t Slot = 0;
};

struct SPARKLE_RENDERER_API MeshDrawSkinning final
{
	std::uint64_t SkeletonAssetId = 0;
	std::uint32_t JointMatrixOffset = (std::numeric_limits<std::uint32_t>::max)();
};

struct SPARKLE_RENDERER_API MeshDrawSourceIdentity final
{
	std::uint32_t SourceInstanceIndex = 0;
	std::uint64_t MeshAssetId = 0;
};

struct SPARKLE_RENDERER_API MeshDrawGeometry final
{
	RenderMeshKind MeshKind = RenderMeshKind::Static;
	const GPUMesh* GpuMesh = nullptr;
};

struct SPARKLE_RENDERER_API MeshDraw
{
	MeshDrawTransform Transform;
	MeshDrawMaterial Material;
	MeshDrawSkinning Skinning;
	MeshDrawSourceIdentity Source;
	MeshDrawGeometry Geometry;
};

struct SPARKLE_RENDERER_API MeshInstanceBatch
{
	const GPUMesh* gpuMesh = nullptr;
	std::uint32_t materialSlot = 0;
	std::uint32_t firstInstance = 0;
	std::uint32_t instanceCount = 0;
	RenderMeshKind meshKind = RenderMeshKind::Static;
	MeshInstanceBatchSource source = MeshInstanceBatchSource::AutoBatch;
};
