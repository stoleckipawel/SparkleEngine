#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanBindingLayout.h"

#include "Pipeline/RhiShaderBindingReflection.h"
#include "ShaderParameters/PassParameterLayout.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <map>
#include <string_view>

static const auto g_vulkanBindingLayoutLogger = Logging::GetOrCreateLogger("RHI.Vulkan.BindingLayout");

class VulkanBindingLayoutCompilerImpl final
{
  public:
	static std::unique_ptr<VulkanBindingLayout> Compile(VulkanRhi& rhi, const RenderBindingLayoutCompileDesc& desc)
	{
		assert(desc.ParameterLayout != nullptr);
		assert(!desc.Shaders.empty());
		const std::vector<PassParameterDesc>& parameters = desc.ParameterLayout->GetParameters();
		std::vector<CompiledBinding> bindings;
		std::vector<std::string> bindingNames;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::map<std::uint32_t, std::vector<PendingDescriptorBinding>> descriptorBindingsBySet;
		std::vector<VkSampler> immutableSamplers;

		bindings.reserve(parameters.size());
		bindingNames.reserve(parameters.size());

		for (std::size_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
		{
			const PassParameterDesc& bindingRecord = parameters[parameterIndex];
			if (bindingRecord.Kind == ShaderParameterSemanticKind::RenderTarget ||
			    bindingRecord.Kind == ShaderParameterSemanticKind::DepthTarget)
			{
				continue;
			}

			const std::string_view bindingName = bindingRecord.Name;
			const std::vector<RhiReflectedBindingLocation> reflectedLocations = RhiShaderBindingReflection::ResolveLocations(
			    desc.Shaders,
			    *desc.ParameterLayout,
			    bindingName,
			    bindingRecord.Kind);
			if (reflectedLocations.size() != 1u)
			{
				Diagnostics::Fatal(
				    g_vulkanBindingLayoutLogger,
				    __FILE__,
				    __LINE__,
				    std::format("Vulkan shader parameter '{}' resolves to {} distinct descriptor locations.", bindingName, reflectedLocations.size()));
			}
			const RhiBindingPoint bindingPoint = reflectedLocations.front().BindingPoint;
			bindingNames.emplace_back(bindingName);

			CompiledBinding compiledBinding{};
			compiledBinding.Name = bindingNames.back().c_str();
			compiledBinding.Type = ToCompiledBindingType(bindingRecord.Kind, desc.InlineUniformDataAsPushConstants);
			compiledBinding.SemanticKind = bindingRecord.Kind;
			compiledBinding.BindingIndex = static_cast<std::uint32_t>(parameterIndex);
			compiledBinding.BindingPoint = bindingPoint;
			compiledBinding.VisibilityMask = reflectedLocations.front().VisibilityMask;
			compiledBinding.DescriptorCount = std::max(1u, bindingRecord.ArrayCount);
			compiledBinding.PushConstantCount = bindingRecord.ValueSizeInBytes / sizeof(std::uint32_t);
			compiledBinding.Bindless.BindlessEligible = bindingRecord.ArrayCount > 1u;
			compiledBinding.Bindless.ReservedDescriptorCount =
			    compiledBinding.Bindless.BindlessEligible ? compiledBinding.DescriptorCount : 0u;
			bindings.push_back(compiledBinding);

			if (compiledBinding.Type == CompiledBindingType::PushConstants)
			{
				pushConstantRanges.push_back(VkPushConstantRange{
				    .stageFlags = ToVkShaderStages(compiledBinding.VisibilityMask),
				    .offset = 0,
				    .size = bindingRecord.ValueSizeInBytes});
				continue;
			}

			const VkDescriptorType descriptorType = ToVkDescriptorType(bindingRecord.Kind, rhi);
			if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
			{
				continue;
			}
			if (compiledBinding.Bindless.BindlessEligible && !rhi.GetFeatureStatus().EnabledPartiallyBoundDescriptorArrays)
			{
				Diagnostics::Fatal(
				    g_vulkanBindingLayoutLogger,
				    __FILE__,
				    __LINE__,
				    std::format("Vulkan shader descriptor array '{}' cannot be partially bound on this device.", bindingName));
			}

			UpsertDescriptorBinding(
			    rhi,
			    descriptorBindingsBySet[bindingPoint.Set],
			    VkDescriptorSetLayoutBinding{
			        .binding = bindingPoint.Binding,
			        .descriptorType = descriptorType,
			        .descriptorCount = std::max(1u, bindingRecord.ArrayCount),
			        .stageFlags = ToVkShaderStages(compiledBinding.VisibilityMask),
			        .pImmutableSamplers = nullptr},
			    compiledBinding.Bindless.BindlessEligible ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0u);
		}

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		for (auto& [setIndex, descriptorBindings] : descriptorBindingsBySet)
		{
			if (descriptorSetLayouts.size() <= setIndex)
			{
				descriptorSetLayouts.resize(static_cast<std::size_t>(setIndex) + 1u, VK_NULL_HANDLE);
			}
			std::ranges::sort(
			    descriptorBindings,
			    [](const PendingDescriptorBinding& lhs, const PendingDescriptorBinding& rhs) { return lhs.Binding.binding < rhs.Binding.binding; });

			std::vector<VkDescriptorSetLayoutBinding> nativeBindings;
			std::vector<VkSampler> nativeImmutableSamplers;
			std::vector<VkDescriptorBindingFlags> nativeBindingFlags;
			nativeBindings.reserve(descriptorBindings.size());
			nativeImmutableSamplers.reserve(descriptorBindings.size());
			nativeBindingFlags.reserve(descriptorBindings.size());
			for (const PendingDescriptorBinding& descriptorBinding : descriptorBindings)
			{
				nativeBindings.push_back(descriptorBinding.Binding);
				nativeBindingFlags.push_back(descriptorBinding.BindingFlags);
				if (descriptorBinding.ImmutableSampler != VK_NULL_HANDLE)
				{
					nativeImmutableSamplers.push_back(descriptorBinding.ImmutableSampler);
					nativeBindings.back().pImmutableSamplers = &nativeImmutableSamplers.back();
				}
			}

			const bool hasBindingFlags = std::any_of(
			    nativeBindingFlags.begin(),
			    nativeBindingFlags.end(),
			    [](VkDescriptorBindingFlags flags) noexcept { return flags != 0; });
			const VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			    .pNext = nullptr,
			    .bindingCount = static_cast<std::uint32_t>(nativeBindingFlags.size()),
			    .pBindingFlags = nativeBindingFlags.empty() ? nullptr : nativeBindingFlags.data()};
			const VkDescriptorSetLayoutCreateInfo createInfo{
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			    .pNext = hasBindingFlags ? static_cast<const void*>(&bindingFlagsCreateInfo) : nullptr,
			    .flags = 0,
			    .bindingCount = static_cast<std::uint32_t>(nativeBindings.size()),
			    .pBindings = nativeBindings.data()};
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			const VkResult result = vkCreateDescriptorSetLayout(rhi.GetDevice(), &createInfo, nullptr, &descriptorSetLayout);
			if (!VulkanResult::Succeeded(result))
			{
				Diagnostics::Fatal(
				    g_vulkanBindingLayoutLogger,
				    __FILE__,
				    __LINE__,
				    VulkanResult::FormatFailure("vkCreateDescriptorSetLayout", result));
			}
			for (VkSampler sampler : nativeImmutableSamplers)
			{
				immutableSamplers.push_back(sampler);
			}
			descriptorSetLayouts[setIndex] = descriptorSetLayout;
		}

		for (VkDescriptorSetLayout& descriptorSetLayout : descriptorSetLayouts)
		{
			if (descriptorSetLayout != VK_NULL_HANDLE)
			{
				continue;
			}

			const VkDescriptorSetLayoutCreateInfo createInfo{
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			    .pNext = nullptr,
			    .flags = 0,
			    .bindingCount = 0,
			    .pBindings = nullptr};
			const VkResult result = vkCreateDescriptorSetLayout(rhi.GetDevice(), &createInfo, nullptr, &descriptorSetLayout);
			if (!VulkanResult::Succeeded(result))
			{
				Diagnostics::Fatal(
				    g_vulkanBindingLayoutLogger,
				    __FILE__,
				    __LINE__,
				    VulkanResult::FormatFailure("vkCreateDescriptorSetLayout", result));
			}
		}

		return std::make_unique<VulkanBindingLayout>(
		    rhi.GetDevice(),
		    *desc.ParameterLayout,
		    std::move(descriptorSetLayouts),
		    std::move(immutableSamplers),
		    std::move(pushConstantRanges),
		    std::move(bindings),
		    std::move(bindingNames));
	}

  private:
	struct PendingDescriptorBinding final
	{
		VkDescriptorSetLayoutBinding Binding = {};
		VkDescriptorBindingFlags BindingFlags = 0;
		VkSampler ImmutableSampler = VK_NULL_HANDLE;
	};
	static CompiledBindingType ToCompiledBindingType(ShaderParameterSemanticKind semanticKind, bool inlineUniformDataAsPushConstants) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return inlineUniformDataAsPushConstants ? CompiledBindingType::PushConstants : CompiledBindingType::ConstantBuffer;
			case ShaderParameterSemanticKind::ReadTexture:
			case ShaderParameterSemanticKind::ReadBuffer:
				return CompiledBindingType::ReadOnlyResourceTable;
			case ShaderParameterSemanticKind::RWTexture:
			case ShaderParameterSemanticKind::RWBuffer:
				return CompiledBindingType::ReadWriteResourceTable;
			case ShaderParameterSemanticKind::SamplerSet:
				return CompiledBindingType::SamplerTable;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return CompiledBindingType::AccelerationStructure;
			default:
				return CompiledBindingType::ReadOnlyResourceTable;
		}
	}

	static void UpsertDescriptorBinding(
	    VulkanRhi& rhi,
	    std::vector<PendingDescriptorBinding>& descriptorBindings,
	    VkDescriptorSetLayoutBinding binding,
	    VkDescriptorBindingFlags bindingFlags) noexcept
	{
		for (PendingDescriptorBinding& existingBinding : descriptorBindings)
		{
			if (existingBinding.Binding.binding == binding.binding && existingBinding.Binding.descriptorType == binding.descriptorType)
			{
				existingBinding.Binding.descriptorCount = std::max(existingBinding.Binding.descriptorCount, binding.descriptorCount);
				existingBinding.Binding.stageFlags |= binding.stageFlags;
				existingBinding.BindingFlags |= bindingFlags;
				return;
			}
		}

		PendingDescriptorBinding pendingBinding{.Binding = binding, .BindingFlags = bindingFlags};
		if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
		{
			pendingBinding.ImmutableSampler = CreateImmutableSampler(rhi);
		}
		descriptorBindings.push_back(pendingBinding);
	}

	static VkSampler CreateImmutableSampler(VulkanRhi& rhi) noexcept
	{
		const VkSamplerCreateInfo createInfo{
		    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		    .pNext = nullptr,
		    .flags = 0,
		    .magFilter = VK_FILTER_LINEAR,
		    .minFilter = VK_FILTER_LINEAR,
		    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		    .mipLodBias = 0.0f,
		    .anisotropyEnable = VK_FALSE,
		    .maxAnisotropy = 1.0f,
		    .compareEnable = VK_FALSE,
		    .compareOp = VK_COMPARE_OP_ALWAYS,
		    .minLod = 0.0f,
		    .maxLod = VK_LOD_CLAMP_NONE,
		    .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		    .unnormalizedCoordinates = VK_FALSE};

		VkSampler sampler = VK_NULL_HANDLE;
		const VkResult result = vkCreateSampler(rhi.GetDevice(), &createInfo, nullptr, &sampler);
		if (!VulkanResult::Succeeded(result))
		{
			Diagnostics::Fatal(
			    g_vulkanBindingLayoutLogger,
			    __FILE__,
			    __LINE__,
			    VulkanResult::FormatFailure("vkCreateSampler", result));
		}
		return sampler;
	}

	static VkDescriptorType ToVkDescriptorType(ShaderParameterSemanticKind semanticKind, const VulkanRhi& rhi) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ShaderParameterSemanticKind::ReadTexture:
				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case ShaderParameterSemanticKind::ReadBuffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case ShaderParameterSemanticKind::RWTexture:
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			case ShaderParameterSemanticKind::RWBuffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case ShaderParameterSemanticKind::SamplerSet:
				return VK_DESCRIPTOR_TYPE_SAMPLER;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return rhi.GetRayTracingCapabilities().Groups.Provider.SelectedTopLevelProvider ==
				        ERhiRayTracingTopLevelProvider::PartitionedTlas
				    ? VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV
				    : VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			default:
				return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	static VkShaderStageFlags ToVkShaderStages(ShaderStageMask visibilityMask) noexcept
	{
		VkShaderStageFlags result = 0;
		if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Vertex))
		{
			result |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Pixel))
		{
			result |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Compute))
		{
			result |= VK_SHADER_STAGE_COMPUTE_BIT;
		}
		return result != 0 ? result : VK_SHADER_STAGE_ALL;
	}
};

VulkanBindingLayout::VulkanBindingLayout(
    VkDevice device,
    const PassParameterLayout& parameterLayout,
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
    std::vector<VkSampler> immutableSamplers,
    std::vector<VkPushConstantRange> pushConstantRanges,
    std::vector<CompiledBinding> bindings,
    std::vector<std::string> bindingNames) noexcept :
	RenderBindingLayout(parameterLayout, std::move(bindings), std::move(bindingNames)), m_device(device),
	m_descriptorSetLayouts(std::move(descriptorSetLayouts)), m_immutableSamplers(std::move(immutableSamplers)),
	m_pushConstantRanges(std::move(pushConstantRanges))
{
}

VulkanBindingLayout::~VulkanBindingLayout() noexcept
{
	if (m_device == VK_NULL_HANDLE)
	{
		return;
	}

	for (VkDescriptorSetLayout descriptorSetLayout : m_descriptorSetLayouts)
	{
		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);
		}
	}
	for (VkSampler sampler : m_immutableSamplers)
	{
		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_device, sampler, nullptr);
		}
	}
}

std::unique_ptr<VulkanBindingLayout> VulkanBindingLayoutCompiler::Compile(VulkanRhi& rhi, const RenderBindingLayoutCompileDesc& desc)
{
	return VulkanBindingLayoutCompilerImpl::Compile(rhi, desc);
}
