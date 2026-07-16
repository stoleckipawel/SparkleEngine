#pragma once

#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"
#include "RhiClassicTlasDesc.h"

#include <cstdint>
#include <string_view>

class SPARKLE_RHI_API RhiClassicTlasService
{
  public:
	virtual ~RhiClassicTlasService() noexcept = default;

	virtual RhiRayTracingAccelerationStructurePrebuildInfo GetClassicTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept = 0;
	virtual RhiOwnedResourceHandle CreateClassicTopLevelAccelerationStructureInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) = 0;
};
