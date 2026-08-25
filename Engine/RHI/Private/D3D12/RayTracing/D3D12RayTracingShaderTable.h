#pragma once

#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "RayTracing/RhiRayTracingPipelineDesc.h"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

class D3D12Rhi;
class D3D12RayTracingPipeline;

class D3D12RayTracingShaderTable final : public RayTracingShaderTable
{
public:
	D3D12RayTracingShaderTable(D3D12Rhi& rhi, const RayTracingShaderTableDesc& desc);

	RhiResourceHandle GetResource() const noexcept override { return RhiResourceHandle{m_allocation.get()}; }
	RhiRayTracingShaderTableRegion GetRayGenerationRegion() const noexcept override { return m_rayGeneration; }
	RhiRayTracingShaderTableRegion GetMissRegion() const noexcept override { return m_miss; }
	RhiRayTracingShaderTableRegion GetHitGroupRegion() const noexcept override { return m_hitGroup; }
	RhiRayTracingShaderTableRegion GetCallableRegion() const noexcept override { return m_callable; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const noexcept;

private:
	static std::vector<std::byte> CollectShaderIdentifiers(
	    const D3D12RayTracingPipeline& pipeline,
	    std::span<const RhiRayTracingShaderRecord> records);

	std::unique_ptr<D3D12GpuAllocationRecord> m_allocation;
	RhiRayTracingShaderTableRegion m_rayGeneration;
	RhiRayTracingShaderTableRegion m_miss;
	RhiRayTracingShaderTableRegion m_hitGroup;
	RhiRayTracingShaderTableRegion m_callable;
};
