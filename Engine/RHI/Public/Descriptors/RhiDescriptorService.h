#pragma once

#include "../Bindings/RenderBindingSet.h"
#include "../Commands/RenderCommandList.h"
#include "../Core/RhiCapabilities.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Resources/RhiResourceView.h"
#include "../Samplers/RhiSamplerDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class ERhiDescriptorUsageStatus : std::uint8_t
{
	Available = 0,
	Unsupported = 1,
	Unavailable = 2
};

struct RhiDescriptorAllocatorUsage
{
	ERhiDescriptorAllocatorType Type = ERhiDescriptorAllocatorType::ShaderResource;
	ERhiDescriptorUsageStatus Status = ERhiDescriptorUsageStatus::Unavailable;
	std::string Name;
	std::uint32_t Capacity = 0;
	std::uint32_t Allocated = 0;
	std::uint32_t Free = 0;
	std::uint32_t HighWatermark = 0;
	float OccupancyRatio = 0.0f;
	std::string Reason;
};

struct RhiDescriptorUsageSnapshot
{
	ERhiDescriptorModel DescriptorModel = ERhiDescriptorModel::Unknown;
	std::vector<RhiDescriptorAllocatorUsage> Allocators;
};

constexpr const char* RhiDescriptorAllocatorTypeToString(ERhiDescriptorAllocatorType type) noexcept
{
	switch (type)
	{
		case ERhiDescriptorAllocatorType::ShaderResource:
			return "ShaderResource";
		case ERhiDescriptorAllocatorType::Sampler:
			return "Sampler";
		case ERhiDescriptorAllocatorType::RenderTarget:
			return "RenderTarget";
		case ERhiDescriptorAllocatorType::DepthStencil:
			return "DepthStencil";
	}

	return "Unknown";
}

constexpr const char* RhiDescriptorUsageStatusToString(ERhiDescriptorUsageStatus status) noexcept
{
	switch (status)
	{
		case ERhiDescriptorUsageStatus::Available:
			return "Available";
		case ERhiDescriptorUsageStatus::Unsupported:
			return "Unsupported";
		case ERhiDescriptorUsageStatus::Unavailable:
			return "Unavailable";
	}

	return "Unknown";
}

class SPARKLE_RHI_API RhiDescriptorService
{
  public:
	virtual ~RhiDescriptorService() noexcept = default;

	virtual std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc) = 0;
	virtual void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept = 0;
	virtual RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType) = 0;
	virtual void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept = 0;
	virtual RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount) = 0;
	virtual RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept = 0;
	virtual void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept = 0;
	virtual void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) = 0;
	virtual void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept = 0;
	virtual RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept = 0;
	virtual RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) = 0;
	virtual void ReleaseResourceView(RhiResourceViewHandle view) noexcept = 0;
	virtual RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept = 0;
	virtual RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept = 0;
	virtual NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept = 0;
	virtual RhiDescriptorUsageSnapshot CaptureDescriptorUsageSnapshot() const = 0;
};
