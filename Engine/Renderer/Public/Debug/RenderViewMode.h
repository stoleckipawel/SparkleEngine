#pragma once

#include <cstdint>

enum class RenderViewMode : std::uint32_t
{
	Lit = 0,
	Wireframe,
	GBufferDiffuse,
	GBufferNormal,
	GBufferRoughness,
	GBufferMetallic,
	GBufferEmissive,
	GBufferAmbientOcclusion,
	GBufferSubsurfaceColor,
	GBufferSubsurfaceStrength,
	DirectDiffuse,
	DirectSpecular,
	DirectSubsurface,
	IndirectDiffuse,
	IndirectSpecular,
	IndirectSubsurface,
	InstanceGroups,
	RayTracingPartitions,
	RayTracingPartitionUpdates,
	RayTracingInstanceMovement,
	RayTracingGpuDrivenUpdates,
	RayTracingTopLevelMode,
	RayTracingNativeOperations,
	RayTracingProviderStatus,
	Count
};
