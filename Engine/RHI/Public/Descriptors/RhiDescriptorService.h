#pragma once

#include "../Bindings/RenderBindingSet.h"
#include "../Core/RhiCapabilities.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Resources/RhiResourceView.h"
#include "../Samplers/RhiSamplerDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <memory>

class RenderCommandList;
class RenderDeviceServices;

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
	virtual RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept = 0;
	virtual RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) = 0;
	virtual bool WriteResourceView(
	    RhiDescriptorTableHandle tableHandle,
	    std::uint32_t descriptorIndex,
	    RhiResourceViewHandle view) noexcept = 0;
	virtual void ReleaseResourceView(RhiResourceViewHandle view) noexcept = 0;
	virtual RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept = 0;
	virtual RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept = 0;

  private:
	friend class RenderDeviceServices;
	virtual void BeginFrame(std::uint32_t frameIndex) noexcept = 0;
};
