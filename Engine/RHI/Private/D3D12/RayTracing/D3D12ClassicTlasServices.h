#pragma once

#include "RayTracing/RhiClassicTlasService.h"

#include <cstdint>
#include <string_view>

class D3D12GpuMemoryAllocator;
class D3D12Rhi;

class D3D12ClassicTlasServices final : public RhiClassicTlasService
{
public:
	D3D12ClassicTlasServices(D3D12Rhi& rhi, D3D12GpuMemoryAllocator& memoryAllocator) noexcept;

	RhiRayTracingAccelerationStructurePrebuildInfo GetClassicTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept override;
	RhiOwnedResourceHandle CreateClassicTopLevelAccelerationStructureInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override;

private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
};
