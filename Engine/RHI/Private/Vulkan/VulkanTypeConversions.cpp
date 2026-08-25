#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanTypeConversions.h"

#include "Core/Public/Diagnostics/Verify.h"

static const auto g_vulkanTypeConversionsLogger = Logging::GetOrCreateLogger("RHI.Vulkan.TypeConversions");

VkFormat VulkanTypeConversions::ToVkFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case PixelFormat::R16G16B16A16_Float:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::R16G16_Float:
			return VK_FORMAT_R16G16_SFLOAT;
		case PixelFormat::R8G8B8A8_UNorm:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case PixelFormat::B8G8R8A8_UNorm:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case PixelFormat::D32_Float:
			return VK_FORMAT_D32_SFLOAT;
		case PixelFormat::R32_Float:
			return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::D24_UNorm_S8_UInt:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::BC1_UNorm:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case PixelFormat::BC1_UNorm_Srgb:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case PixelFormat::BC2_UNorm:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case PixelFormat::BC2_UNorm_Srgb:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case PixelFormat::BC3_UNorm:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case PixelFormat::BC3_UNorm_Srgb:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case PixelFormat::BC4_UNorm:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case PixelFormat::BC4_SNorm:
			return VK_FORMAT_BC4_SNORM_BLOCK;
		case PixelFormat::BC5_UNorm:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case PixelFormat::BC5_SNorm:
			return VK_FORMAT_BC5_SNORM_BLOCK;
		case PixelFormat::BC6H_UF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case PixelFormat::BC7_UNorm:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case PixelFormat::BC7_UNorm_Srgb:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case PixelFormat::Unknown:
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

PixelFormat VulkanTypeConversions::ToPixelFormat(VkFormat format) noexcept
{
	switch (format)
	{
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return PixelFormat::R32G32B32A32_Float;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return PixelFormat::R16G16B16A16_Float;
		case VK_FORMAT_R16G16_SFLOAT:
			return PixelFormat::R16G16_Float;
		case VK_FORMAT_R8G8B8A8_UNORM:
			return PixelFormat::R8G8B8A8_UNorm;
		case VK_FORMAT_R8G8B8A8_SRGB:
			return PixelFormat::R8G8B8A8_UNorm_Srgb;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return PixelFormat::B8G8R8A8_UNorm;
		case VK_FORMAT_B8G8R8A8_SRGB:
			return PixelFormat::B8G8R8A8_UNorm_Srgb;
		case VK_FORMAT_D32_SFLOAT:
			return PixelFormat::D32_Float;
		case VK_FORMAT_R32_SFLOAT:
			return PixelFormat::R32_Float;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return PixelFormat::D24_UNorm_S8_UInt;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			return PixelFormat::BC1_UNorm;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			return PixelFormat::BC1_UNorm_Srgb;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			return PixelFormat::BC2_UNorm;
		case VK_FORMAT_BC2_SRGB_BLOCK:
			return PixelFormat::BC2_UNorm_Srgb;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return PixelFormat::BC3_UNorm;
		case VK_FORMAT_BC3_SRGB_BLOCK:
			return PixelFormat::BC3_UNorm_Srgb;
		case VK_FORMAT_BC4_UNORM_BLOCK:
			return PixelFormat::BC4_UNorm;
		case VK_FORMAT_BC4_SNORM_BLOCK:
			return PixelFormat::BC4_SNorm;
		case VK_FORMAT_BC5_UNORM_BLOCK:
			return PixelFormat::BC5_UNorm;
		case VK_FORMAT_BC5_SNORM_BLOCK:
			return PixelFormat::BC5_SNorm;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			return PixelFormat::BC6H_UF16;
		case VK_FORMAT_BC7_UNORM_BLOCK:
			return PixelFormat::BC7_UNorm;
		case VK_FORMAT_BC7_SRGB_BLOCK:
			return PixelFormat::BC7_UNorm_Srgb;
		default:
			return PixelFormat::Unknown;
	}
}

VkIndexType VulkanTypeConversions::ToVkIndexType(RhiIndexFormat format) noexcept
{
	switch (format)
	{
		case RhiIndexFormat::UInt16:
			return VK_INDEX_TYPE_UINT16;
		case RhiIndexFormat::UInt32:
			return VK_INDEX_TYPE_UINT32;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan index format.");
	}
}

VkCompareOp VulkanTypeConversions::ToVkCompareOp(CompareOp op) noexcept
{
	switch (op)
	{
		case CompareOp::Never:
			return VK_COMPARE_OP_NEVER;
		case CompareOp::Less:
			return VK_COMPARE_OP_LESS;
		case CompareOp::Equal:
			return VK_COMPARE_OP_EQUAL;
		case CompareOp::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareOp::Greater:
			return VK_COMPARE_OP_GREATER;
		case CompareOp::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;
		case CompareOp::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareOp::Always:
			return VK_COMPARE_OP_ALWAYS;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan comparison operation.");
	}
}

VkStencilOp VulkanTypeConversions::ToVkStencilOp(RhiStencilOp op) noexcept
{
	switch (op)
	{
		case RhiStencilOp::Keep:
			return VK_STENCIL_OP_KEEP;
		case RhiStencilOp::Zero:
			return VK_STENCIL_OP_ZERO;
		case RhiStencilOp::Replace:
			return VK_STENCIL_OP_REPLACE;
		case RhiStencilOp::IncrementClamp:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case RhiStencilOp::DecrementClamp:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case RhiStencilOp::Invert:
			return VK_STENCIL_OP_INVERT;
		case RhiStencilOp::IncrementWrap:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case RhiStencilOp::DecrementWrap:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan stencil operation.");
	}
}

VkCullModeFlags VulkanTypeConversions::ToVkCullModeFlags(ERhiCullMode cullMode) noexcept
{
	switch (cullMode)
	{
		case ERhiCullMode::None:
			return VK_CULL_MODE_NONE;
		case ERhiCullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;
		case ERhiCullMode::Back:
			return VK_CULL_MODE_BACK_BIT;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan cull mode.");
	}
}

VkPrimitiveTopology VulkanTypeConversions::ToVkPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	switch (topology)
	{
		case RhiPrimitiveTopology::TriangleList:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan primitive topology.");
	}
}

bool VulkanTypeConversions::IsBufferResourceStateSupported(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
		case ResourceState::ShaderResource:
		case ResourceState::UnorderedAccess:
		case ResourceState::RayTracingAccelerationStructure:
		case ResourceState::RayTracingShaderTable:
		case ResourceState::CopySource:
		case ResourceState::CopyDest:
			return true;
		case ResourceState::Undefined:
		case ResourceState::RenderTarget:
		case ResourceState::DepthWrite:
		case ResourceState::DepthRead:
		case ResourceState::Present:
		case ResourceState::Count:
		default:
			return false;
	}
}

bool VulkanTypeConversions::IsImageResourceStateSupported(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Undefined:
		case ResourceState::Common:
		case ResourceState::RenderTarget:
		case ResourceState::DepthWrite:
		case ResourceState::DepthRead:
		case ResourceState::ShaderResource:
		case ResourceState::UnorderedAccess:
		case ResourceState::CopySource:
		case ResourceState::CopyDest:
		case ResourceState::Present:
			return true;
		case ResourceState::RayTracingAccelerationStructure:
		case ResourceState::RayTracingShaderTable:
		case ResourceState::Count:
		default:
			return false;
	}
}

VulkanResourceStateMapping VulkanTypeConversions::ToResourceStateMapping(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Undefined:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			    .AccessMask = 0,
			    .ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED};
		case ResourceState::Common:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			    .AccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::RenderTarget:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			    .AccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
		case ResourceState::DepthWrite:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			    .AccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		case ResourceState::DepthRead:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT
			        | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			    .AccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
		case ResourceState::ShaderResource:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
			        | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		case ResourceState::UnorderedAccess:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
			        | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::RayTracingAccelerationStructure:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
			        | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::RayTracingShaderTable:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			    .AccessMask = VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,
			    .ImageLayout = VK_IMAGE_LAYOUT_GENERAL};
		case ResourceState::CopySource:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			    .AccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
		case ResourceState::CopyDest:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			    .AccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
		case ResourceState::Present:
			return VulkanResourceStateMapping{
			    .StageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			    .AccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
			    .ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan resource state.");
	}
}

VkBufferCreateInfo VulkanTypeConversions::BuildBufferCreateInfo(const RhiBufferResourceDesc& desc, VkBufferUsageFlags extraUsage) noexcept
{
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | extraUsage;
	switch (desc.Kind)
	{
		case RhiBufferKind::Vertex:
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case RhiBufferKind::Index:
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case RhiBufferKind::Structured:
			usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case RhiBufferKind::Generic:
			if (desc.StrideInBytes > 0)
			{
				usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}
			break;
		default:
			Diagnostics::Fatal(g_vulkanTypeConversionsLogger, __FILE__, __LINE__, "Unsupported Vulkan buffer kind.");
	}
	if (desc.AllowUnorderedAccess)
	{
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (desc.AllowRayTracingBuildInput)
	{
		usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return VkBufferCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = desc.SizeInBytes,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
}

VkImageCreateInfo VulkanTypeConversions::BuildTextureCreateInfo(const RhiTextureResourceDesc& desc, VkImageUsageFlags extraUsage) noexcept
{
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extraUsage;
	if (desc.AllowRenderTarget)
	{
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (desc.AllowDepthStencil)
	{
		usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (desc.AllowUnorderedAccess)
	{
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	return VkImageCreateInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = desc.Dimension == TextureResourceDimension::TextureCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .format = ToVkFormat(desc.Format),
	    .extent = VkExtent3D{.width = desc.Width, .height = desc.Height, .depth = 1},
	    .mipLevels = desc.MipLevels,
	    .arrayLayers = desc.ArraySize,
	    .samples = static_cast<VkSampleCountFlagBits>(desc.SampleCount),
	    .tiling = VK_IMAGE_TILING_OPTIMAL,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
}

VkImageAspectFlags VulkanTypeConversions::ResolveAspectMask(PixelFormat format) noexcept
{
	if (PixelFormatHasStencilAspect(format))
	{
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (IsDepthStencilPixelFormat(format))
	{
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	if (PixelFormatHasColorAspect(format))
	{
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
	Diagnostics::Fatal(
	    g_vulkanTypeConversionsLogger,
	    __FILE__,
	    __LINE__,
	    "Cannot resolve an aspect mask for an unknown Vulkan format.");
}
