#pragma once

#include "../RendererAPI.h"
#include "Renderer/Public/Meshes/GpuMeshHandle.h"
#include "Renderer/Public/SceneData/RenderMeshClassification.h"

#include <DirectXMath.h>
#include <cstdint>
#include <limits>

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

struct SPARKLE_RENDERER_API MeshDrawMorph final
{
	std::uint32_t WeightOffset = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t TargetCount = 0u;
	std::uint32_t VertexCount = 0u;
};

struct SPARKLE_RENDERER_API MeshDrawSourceIdentity final
{
	std::uint32_t GpuSceneSlot = 0;
	std::uint64_t MeshAssetId = 0;
	std::uint32_t MeshGeneration = 0;
};

struct SPARKLE_RENDERER_API MeshDrawGeometry final
{
	RenderMeshKind MeshKind = RenderMeshKind::Static;
	GpuMeshHandle Mesh;
	DirectX::XMFLOAT3 LocalBoundsMin = {};
	DirectX::XMFLOAT3 LocalBoundsMax = {};
	bool HasLocalBounds = false;
};

struct SPARKLE_RENDERER_API MeshDraw
{
	MeshDrawTransform Transform;
	MeshDrawMaterial Material;
	MeshDrawSkinning Skinning;
	MeshDrawMorph Morph;
	MeshDrawSourceIdentity Source;
	MeshDrawGeometry Geometry;
};

struct SPARKLE_RENDERER_API MeshInstanceBatch
{
	GpuMeshHandle Mesh;
	std::uint32_t materialSlot = 0;
	std::uint32_t firstInstance = 0;
	std::uint32_t instanceCount = 0;
	RenderMeshKind meshKind = RenderMeshKind::Static;
	MeshInstanceBatchSource source = MeshInstanceBatchSource::AutoBatch;
};
