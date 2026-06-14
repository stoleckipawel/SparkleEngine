#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

struct RayTracingPtlasFrameGraphResourceBindings
{
	RhiOwnedResourceHandle LogicalUpdateRecords = {};
	RhiOwnedResourceHandle NativeOperationData = {};
	RhiOwnedResourceHandle Scratch = {};

	bool HasLogicalUpdateRecords() const noexcept
	{
		return static_cast<bool>(LogicalUpdateRecords);
	}

	bool HasNativeOperationData() const noexcept
	{
		return static_cast<bool>(NativeOperationData);
	}

	bool HasScratch() const noexcept
	{
		return static_cast<bool>(Scratch);
	}

	bool HasOperationResources() const noexcept
	{
		return HasLogicalUpdateRecords() && HasNativeOperationData() && HasScratch();
	}
};

struct RayTracingSceneFrameData
{
	bool IsAvailable = false;
	RhiOwnedResourceHandle TlasResource = {};
	RhiGpuVirtualAddress TlasGpuAddress = 0;
	std::uint32_t EstimatedInstanceCount = 0;
	RayTracingPtlasFrameGraphResourceBindings PtlasFrameGraphResources = {};

	bool HasBoundTlas() const noexcept
	{
		return IsAvailable && TlasResource && TlasGpuAddress != 0;
	}

	bool HasTraceableInstances() const noexcept
	{
		return HasBoundTlas() && EstimatedInstanceCount > 0;
	}

	bool HasPartitionedTlasOperationResources() const noexcept
	{
		return PtlasFrameGraphResources.HasOperationResources();
	}
};
