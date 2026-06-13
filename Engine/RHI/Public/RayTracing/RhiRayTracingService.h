#pragma once

#include "../Interop/RhiNativeHandles.h"
#include "../Memory/RhiMemoryTypes.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

class SPARKLE_RHI_API RhiRayTracingService
{
  public:
	virtual ~RhiRayTracingService() noexcept = default;

	virtual RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept = 0;
	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount) const noexcept = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) = 0;
};
