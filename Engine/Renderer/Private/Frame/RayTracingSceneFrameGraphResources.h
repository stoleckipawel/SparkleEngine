#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"

struct RayTracingSceneFrameGraphResources final
{
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	FrameGraphBufferHandle PtlasLogicalUpdateRecords = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PtlasNativeOperationData = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PtlasScratch = FrameGraphBufferHandle::Invalid();

	bool HasSceneTlas() const noexcept;
	bool HasPartitionedTlasResources() const noexcept;
};
