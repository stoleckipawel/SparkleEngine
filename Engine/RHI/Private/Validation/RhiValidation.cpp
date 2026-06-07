#include "PCH.h"

#include "RHI/Public/Validation/RhiValidation.h"

#include <cassert>
#include <format>

namespace
{
#ifndef ENGINE_GPU_VALIDATION
#define ENGINE_GPU_VALIDATION 0
#endif

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

bool RhiValidation::IsEnabled() noexcept
{
	return ENGINE_GPU_VALIDATION != 0;
}

void RhiValidation::ReportContractViolation(
    std::string_view owner,
    std::string_view condition,
    std::string_view recommendedFix) noexcept
{
	if (!IsEnabled())
	{
		return;
	}

	static const auto logger = Logging::GetOrCreateLogger("RHI.Validation");
	if (logger != nullptr)
	{
		SPDLOG_LOGGER_ERROR(
		    logger,
		    "RHI validation violation: owner='{}' condition='{}' recommendedFix='{}'",
		    owner,
		    condition,
		    recommendedFix);
	}
	assert(false);
}

bool RhiValidation::ValidateBindingSetDesc(
    const RhiCapabilities& capabilities,
    const RenderBindingSetDesc& desc,
    std::string_view owner) noexcept
{
	if (!IsEnabled())
	{
		return true;
	}

	if (desc.DescriptorCount == 0)
	{
		ReportContractViolation(owner, "binding set descriptor count is zero", "request at least one descriptor or skip binding set creation");
		return false;
	}

	const std::uint32_t descriptorLimit = GetDescriptorLimit(capabilities, desc.DescriptorType);
	if (descriptorLimit != 0 && desc.DescriptorCount > descriptorLimit)
	{
		const std::string condition =
		    std::format("binding set descriptor count {} exceeds backend limit {}", desc.DescriptorCount, descriptorLimit);
		ReportContractViolation(
		    owner,
		    condition,
		    "split the binding set, reduce shader-visible descriptors, or raise the backend capability only after backend allocator support exists");
		return false;
	}

	return true;
}

bool RhiValidation::ValidateBindingSetDescriptorIndex(
    std::uint32_t descriptorIndex,
    std::uint32_t descriptorCount,
    std::string_view owner) noexcept
{
	if (!IsEnabled())
	{
		return true;
	}

	if (descriptorIndex >= descriptorCount)
	{
		const std::string condition =
		    std::format("descriptor index {} is outside binding set descriptor count {}", descriptorIndex, descriptorCount);
		ReportContractViolation(
		    owner,
		    condition,
		    "bind only descriptors declared by the binding set owner or create a larger binding set before writing/binding");
		return false;
	}

	return true;
}

bool RhiValidation::ValidateTextureResourceDesc(
    const RhiCapabilities& capabilities,
    const RhiTextureResourceDesc& desc,
    std::string_view owner) noexcept
{
	if (!IsEnabled())
	{
		return true;
	}

	if (desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		ReportContractViolation(
		    owner,
		    "texture resource descriptor has zero extent or unknown format",
		    "fix the Renderer/FrameGraph resource declaration before requesting backend allocation");
		return false;
	}

	const RhiFormatSupport* const formatSupport = capabilities.FindFormatSupport(desc.Format);
	if (formatSupport == nullptr || !formatSupport->SupportsTexture)
	{
		const std::string condition =
		    std::format("texture format '{}' is not reported as texture-supported by the active backend", PixelFormatName(desc.Format));
		ReportContractViolation(
		    owner,
		    condition,
		    "choose a supported format or update the backend capability report only after backend allocation/view support exists");
		return false;
	}

	if (desc.AllowRenderTarget && !formatSupport->SupportsRenderTarget)
	{
		const std::string condition =
		    std::format("texture format '{}' was requested as render target but backend reports that usage unsupported", PixelFormatName(desc.Format));
		ReportContractViolation(owner, condition, "change the FrameGraph resource kind or select a render-target-capable format");
		return false;
	}

	if (desc.AllowDepthStencil && !formatSupport->SupportsDepthStencil)
	{
		const std::string condition =
		    std::format("texture format '{}' was requested as depth stencil but backend reports that usage unsupported", PixelFormatName(desc.Format));
		ReportContractViolation(owner, condition, "change the FrameGraph resource kind or select a depth-stencil-capable format");
		return false;
	}

	if (desc.AllowUnorderedAccess && !formatSupport->SupportsUnorderedAccess)
	{
		const std::string condition =
		    std::format("texture format '{}' was requested as unordered access but backend reports that usage unsupported", PixelFormatName(desc.Format));
		ReportContractViolation(owner, condition, "remove UAV usage from the pass declaration or select a UAV-capable format");
		return false;
	}

	return true;
}
