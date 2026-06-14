#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanBindingLayout.h"

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageCache.h"
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
		assert(desc.ShaderPackage != nullptr);

		const LoadedShaderPackage& shaderPackage = *desc.ShaderPackage;
		std::vector<CompiledBinding> bindings;
		std::vector<std::string> bindingNames;
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::map<std::uint32_t, std::vector<PendingDescriptorBinding>> descriptorBindingsBySet;
		std::vector<VkSampler> immutableSamplers;

		bindings.reserve(shaderPackage.GetBindingRecords().size());
		bindingNames.reserve(shaderPackage.GetBindingRecords().size());

		for (const CookedShaderBindingRecord& bindingRecord : shaderPackage.GetBindingRecords())
		{
			if (bindingRecord.SemanticKind == ShaderParameterSemanticKind::RenderTarget ||
			    bindingRecord.SemanticKind == ShaderParameterSemanticKind::DepthTarget)
			{
				continue;
			}

			const std::string_view bindingName = shaderPackage.ResolveString(bindingRecord.Name);
			const PassParameterDesc* parameterDesc = FindParameter(*desc.ParameterLayout, bindingName);
			const ReflectedBindingLocation location = ResolveBindingLocation(shaderPackage, bindingRecord, bindingName, parameterDesc);
			bindingNames.emplace_back(bindingName);

			CompiledBinding compiledBinding{};
			compiledBinding.Name = bindingNames.back().c_str();
			compiledBinding.Type = ToCompiledBindingType(bindingRecord.SemanticKind, desc.InlineUniformDataAsPushConstants);
			compiledBinding.SemanticKind = bindingRecord.SemanticKind;
			compiledBinding.BindingIndex = bindingRecord.LogicalBindingIndex;
			compiledBinding.BindingPoint = RhiBindingPoint{.Set = location.Set, .Binding = location.Binding};
			compiledBinding.VisibilityMask = bindingRecord.VisibilityMask;
			compiledBinding.DescriptorCount = std::max(1u, bindingRecord.ArrayCount);
			compiledBinding.PushConstantCount = bindingRecord.ValueSizeInBytes / sizeof(std::uint32_t);
			bindings.push_back(compiledBinding);

			if (compiledBinding.Type == CompiledBindingType::PushConstants)
			{
				pushConstantRanges.push_back(VkPushConstantRange{
				    .stageFlags = ToVkShaderStages(bindingRecord.VisibilityMask),
				    .offset = 0,
				    .size = bindingRecord.ValueSizeInBytes});
				continue;
			}

			const VkDescriptorType descriptorType = ToVkDescriptorType(bindingRecord.SemanticKind, rhi);
			if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
			{
				continue;
			}

			UpsertDescriptorBinding(
			    rhi,
			    descriptorBindingsBySet[location.Set],
			    VkDescriptorSetLayoutBinding{
			        .binding = location.Binding,
			        .descriptorType = descriptorType,
			        .descriptorCount = std::max(1u, bindingRecord.ArrayCount),
			        .stageFlags = ToVkShaderStages(bindingRecord.VisibilityMask),
			        .pImmutableSamplers = nullptr});
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
			std::vector<MutableDescriptorTypeStorage> mutableDescriptorTypes;
			std::vector<VkMutableDescriptorTypeListEXT> mutableDescriptorTypeLists;
			nativeBindings.reserve(descriptorBindings.size());
			nativeImmutableSamplers.reserve(descriptorBindings.size());
			mutableDescriptorTypes.resize(descriptorBindings.size());
			mutableDescriptorTypeLists.resize(descriptorBindings.size());
			for (const PendingDescriptorBinding& descriptorBinding : descriptorBindings)
			{
				nativeBindings.push_back(descriptorBinding.Binding);
				if (descriptorBinding.ImmutableSampler != VK_NULL_HANDLE)
				{
					nativeImmutableSamplers.push_back(descriptorBinding.ImmutableSampler);
					nativeBindings.back().pImmutableSamplers = &nativeImmutableSamplers.back();
				}
				if (descriptorBinding.Binding.descriptorType == VK_DESCRIPTOR_TYPE_MUTABLE_EXT)
				{
					const std::size_t nativeBindingIndex = nativeBindings.size() - 1u;
					mutableDescriptorTypes[nativeBindingIndex] = MutableDescriptorTypeStorage{
					    VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
					    VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV};
					mutableDescriptorTypeLists[nativeBindingIndex] = VkMutableDescriptorTypeListEXT{
					    .descriptorTypeCount = static_cast<std::uint32_t>(mutableDescriptorTypes[nativeBindingIndex].size()),
					    .pDescriptorTypes = mutableDescriptorTypes[nativeBindingIndex].data()};
				}
			}

			const bool hasMutableDescriptorTypes = std::any_of(
			    mutableDescriptorTypeLists.begin(),
			    mutableDescriptorTypeLists.end(),
			    [](const VkMutableDescriptorTypeListEXT& typeList) noexcept { return typeList.descriptorTypeCount > 0; });
			const VkMutableDescriptorTypeCreateInfoEXT mutableDescriptorCreateInfo{
			    .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
			    .pNext = nullptr,
			    .mutableDescriptorTypeListCount = static_cast<std::uint32_t>(mutableDescriptorTypeLists.size()),
			    .pMutableDescriptorTypeLists = mutableDescriptorTypeLists.empty() ? nullptr : mutableDescriptorTypeLists.data()};
			const VkDescriptorSetLayoutCreateInfo createInfo{
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			    .pNext = hasMutableDescriptorTypes ? &mutableDescriptorCreateInfo : nullptr,
			    .flags = 0,
			    .bindingCount = static_cast<std::uint32_t>(nativeBindings.size()),
			    .pBindings = nativeBindings.data()};
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			const VkResult result = vkCreateDescriptorSetLayout(rhi.GetDevice(), &createInfo, nullptr, &descriptorSetLayout);
			if (!VulkanResult::Succeeded(result))
			{
				Diagnostics::Fail(
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
				Diagnostics::Fail(
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
	struct ReflectedBindingLocation final
	{
		std::uint32_t Set = 0;
		std::uint32_t Binding = 0;
	};

	struct PendingDescriptorBinding final
	{
		VkDescriptorSetLayoutBinding Binding = {};
		VkSampler ImmutableSampler = VK_NULL_HANDLE;
	};
	using MutableDescriptorTypeStorage = std::array<VkDescriptorType, 2>;

	static const PassParameterDesc* FindParameter(const PassParameterLayout& layout, std::string_view name) noexcept
	{
		for (const PassParameterDesc& parameter : layout.GetParameters())
		{
			if (parameter.Name == name)
			{
				return &parameter;
			}
		}
		return nullptr;
	}

	static bool ResourceKindMatches(CookedShaderResourceKind kind, ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return kind == CookedShaderResourceKind::ConstantBuffer || kind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture:
				return kind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return kind == CookedShaderResourceKind::StructuredBuffer || kind == CookedShaderResourceKind::ByteAddressBuffer ||
				       kind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::RWTexture:
				return kind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return kind == CookedShaderResourceKind::RWStructuredBuffer || kind == CookedShaderResourceKind::RWByteAddressBuffer ||
				       kind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet:
				return kind == CookedShaderResourceKind::Sampler;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return kind == CookedShaderResourceKind::AccelerationStructure;
			default:
				return false;
		}
	}

	static ReflectedBindingLocation ResolveBindingLocation(
	    const LoadedShaderPackage& shaderPackage,
	    const CookedShaderBindingRecord& bindingRecord,
	    std::string_view bindingName,
	    const PassParameterDesc* parameterDesc) noexcept
	{
		const std::string_view shaderName = parameterDesc != nullptr ? parameterDesc->GetShaderName() : bindingName;
		const std::vector<CookedShaderBinaryRecord>& binaryRecords = shaderPackage.GetBinaryRecords();
		const std::vector<CookedShaderReflectionRecord>& reflectionRecords = shaderPackage.GetReflectionRecords();
		const std::vector<CookedShaderResourceBindingRecord>& resourceBindings = shaderPackage.GetResourceBindings();

		for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size() && reflectionIndex < binaryRecords.size(); ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (binaryRecord.Format != CookedShaderBinaryFormat::SpirV || !HasAnyShaderStageMask(bindingRecord.VisibilityMask, ToShaderStageMask(binaryRecord.Stage)))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (bindingIndex >= resourceBindings.size())
				{
					break;
				}

				const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[bindingIndex];
				if (!ResourceKindMatches(resourceBinding.Kind, bindingRecord.SemanticKind))
				{
					continue;
				}

				const std::string_view resourceName = shaderPackage.ResolveString(
				    CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (resourceName == shaderName)
				{
					return ReflectedBindingLocation{.Set = resourceBinding.Set, .Binding = resourceBinding.Slot};
				}
			}
		}

		return ReflectedBindingLocation{.Set = 0, .Binding = bindingRecord.LogicalBindingIndex};
	}

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
				return CompiledBindingType::ReadOnlyAddress;
			default:
				return CompiledBindingType::ReadOnlyResourceTable;
		}
	}

	static void UpsertDescriptorBinding(
	    VulkanRhi& rhi,
	    std::vector<PendingDescriptorBinding>& descriptorBindings,
	    VkDescriptorSetLayoutBinding binding) noexcept
	{
		for (PendingDescriptorBinding& existingBinding : descriptorBindings)
		{
			if (existingBinding.Binding.binding == binding.binding && existingBinding.Binding.descriptorType == binding.descriptorType)
			{
				existingBinding.Binding.descriptorCount = std::max(existingBinding.Binding.descriptorCount, binding.descriptorCount);
				existingBinding.Binding.stageFlags |= binding.stageFlags;
				return;
			}
		}

		PendingDescriptorBinding pendingBinding{.Binding = binding};
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
			Diagnostics::Fail(
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
				return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
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
	m_device(device), m_parameterLayout(&parameterLayout), m_descriptorSetLayouts(std::move(descriptorSetLayouts)),
	m_immutableSamplers(std::move(immutableSamplers)), m_pushConstantRanges(std::move(pushConstantRanges)), m_bindings(std::move(bindings)),
	m_bindingNames(std::move(bindingNames))
{
	for (std::size_t bindingIndex = 0; bindingIndex < m_bindings.size() && bindingIndex < m_bindingNames.size(); ++bindingIndex)
	{
		m_bindings[bindingIndex].Name = m_bindingNames[bindingIndex].c_str();
	}
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

const PassParameterLayout& VulkanBindingLayout::GetParameterLayout() const noexcept
{
	return *m_parameterLayout;
}

const CompiledBinding* VulkanBindingLayout::GetBindings() const noexcept
{
	return m_bindings.data();
}

std::size_t VulkanBindingLayout::GetBindingCount() const noexcept
{
	return m_bindings.size();
}

const CompiledBinding* VulkanBindingLayout::FindBinding(const char* name) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	for (const CompiledBinding& binding : m_bindings)
	{
		if (binding.Name != nullptr && std::string_view(binding.Name) == name)
		{
			return &binding;
		}
	}

	return nullptr;
}

std::unique_ptr<VulkanBindingLayout> VulkanBindingLayoutCompiler::Compile(VulkanRhi& rhi, const RenderBindingLayoutCompileDesc& desc)
{
	return VulkanBindingLayoutCompilerImpl::Compile(rhi, desc);
}
