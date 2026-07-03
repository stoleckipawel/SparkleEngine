#pragma once

#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>

namespace RhiContract
{
	bool IsBindingSetDescUsable(const RhiCapabilities& capabilities, const RenderBindingSetDesc& desc) noexcept;
	bool IsBindingSetDescriptorIndexValid(std::uint32_t descriptorIndex, std::uint32_t descriptorCount) noexcept;
	bool IsTextureResourceDescUsable(const RhiCapabilities& capabilities, const RhiTextureResourceDesc& desc) noexcept;
	bool IsRayTracingGeometryDescUsable(const RhiRayTracingGeometryDesc& geometry) noexcept;
	bool IsRayTracingInstanceListUsable(const RhiRayTracingInstanceDesc* instances, std::uint32_t instanceCount) noexcept;
	bool IsRayTracingGpuAddressPresent(RhiGpuVirtualAddress gpuAddress) noexcept;
	bool IsRayTracingBufferSizeUsable(std::uint64_t sizeInBytes, std::uint64_t alignmentInBytes) noexcept;
}
