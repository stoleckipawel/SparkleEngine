#include "PCH.h"

#include "Validation/RhiContract.h"

namespace
{
	std::uint32_t GetDescriptorLimit(const RhiCapabilities& capabilities, ERhiDescriptorAllocatorType descriptorType) noexcept
	{
		switch (descriptorType)
		{
			case ERhiDescriptorAllocatorType::Sampler:
				return capabilities.BindingLimits.MaxSamplerDescriptors;
			case ERhiDescriptorAllocatorType::ShaderResource:
			case ERhiDescriptorAllocatorType::RenderTarget:
			case ERhiDescriptorAllocatorType::DepthStencil:
			default:
				return capabilities.BindingLimits.MaxDescriptorTableEntries;
		}
	}
}

bool RhiContract::IsBindingSetDescUsable(const RhiCapabilities& capabilities, const RenderBindingSetDesc& desc) noexcept
{
	const std::uint32_t descriptorLimit = GetDescriptorLimit(capabilities, desc.DescriptorType);
	return desc.DescriptorCount != 0 && (descriptorLimit == 0 || desc.DescriptorCount <= descriptorLimit);
}

bool RhiContract::IsBindingSetDescriptorIndexValid(std::uint32_t descriptorIndex, std::uint32_t descriptorCount) noexcept
{
	return descriptorIndex < descriptorCount;
}

bool RhiContract::IsTextureResourceDescUsable(const RhiCapabilities& capabilities, const RhiTextureResourceDesc& desc) noexcept
{
	if (desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		return false;
	}

	const RhiFormatSupport* const formatSupport = capabilities.FindFormatSupport(desc.Format);
	return formatSupport != nullptr && formatSupport->SupportsTexture &&
	       (!desc.AllowRenderTarget || formatSupport->SupportsRenderTarget) &&
	       (!desc.AllowDepthStencil || formatSupport->SupportsDepthStencil) &&
	       (!desc.AllowUnorderedAccess || formatSupport->SupportsUnorderedAccess);
}

bool RhiContract::IsRayTracingGeometryDescUsable(const RhiRayTracingGeometryDesc& geometry) noexcept
{
	return geometry.VertexBuffer != 0 && geometry.IndexBuffer != 0 && geometry.VertexStrideInBytes >= sizeof(float) * 3u &&
	       geometry.VertexCount != 0 && geometry.IndexCount != 0 && geometry.IndexCount % 3u == 0;
}

bool RhiContract::IsRayTracingInstanceListUsable(const RhiRayTracingInstanceDesc* instances, std::uint32_t instanceCount) noexcept
{
	if (instances == nullptr || instanceCount == 0)
	{
		return false;
	}

	for (std::uint32_t index = 0; index < instanceCount; ++index)
	{
		const RhiRayTracingInstanceDesc& instance = instances[index];
		if (instance.AccelerationStructure == 0 || instance.InstanceMask == 0)
		{
			return false;
		}
	}

	return true;
}

bool RhiContract::IsRayTracingGpuAddressPresent(RhiGpuVirtualAddress gpuAddress) noexcept
{
	return gpuAddress != 0;
}

bool RhiContract::IsRayTracingBufferSizeUsable(std::uint64_t sizeInBytes, std::uint64_t alignmentInBytes) noexcept
{
	return sizeInBytes != 0 && (alignmentInBytes == 0 || sizeInBytes % alignmentInBytes == 0);
}
