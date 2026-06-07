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

struct SPARKLE_RENDERER_API MeshDraw
{
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	std::uint32_t materialSlot = 0;
	std::uint64_t skeletonAssetId = 0;
	std::uint32_t jointMatrixOffset = (std::numeric_limits<std::uint32_t>::max)();
	RenderMeshKind meshKind = RenderMeshKind::Static;
	const GPUMesh* gpuMesh = nullptr;
};

struct SPARKLE_RENDERER_API MeshInstanceBatch
{
	const GPUMesh* gpuMesh = nullptr;
	std::uint32_t materialSlot = 0;
	std::uint32_t firstInstance = 0;
	std::uint32_t instanceCount = 0;
	MeshInstanceBatchSource source = MeshInstanceBatchSource::AutoBatch;
};
