#pragma once

#include "Renderer/Public/Meshes/GpuMeshHandle.h"
#include "Renderer/Public/SceneData/RenderMeshClassification.h"

#include <cstdint>

enum class RenderMaterialClassification : std::uint8_t
{
	Opaque,
	AlphaTested,
	Transparent,
	Rejected
};

enum class MeshInstanceBatchSource : std::uint32_t
{
	PreservedGroup = 0,
	AuthoredGroup = 1,
	AutoBatch = 2,
	SingleInstance = 3,
};

struct MeshInstanceBatch final
{
	GpuMeshHandle Mesh;
	std::uint32_t materialSlot = 0;
	std::uint32_t firstInstance = 0;
	std::uint32_t instanceCount = 0;
	RenderMeshKind meshKind = RenderMeshKind::Static;
	RenderMaterialClassification materialClassification = RenderMaterialClassification::Rejected;
	MeshInstanceBatchSource source = MeshInstanceBatchSource::AutoBatch;
};
