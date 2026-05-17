#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanBindingLayout.h"

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageCache.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"

#include <algorithm>
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
		std::map<std::uint32_t, std::vector<VkDescriptorSetLayoutBinding>> descriptorBindingsBySet;

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

			const VkDescriptorType descriptorType = ToVkDescriptorType(bindingRecord.SemanticKind);
			if (descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
			{
				continue;
			}

			UpsertDescriptorBinding(
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
			    [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs) { return lhs.binding < rhs.binding; });

			const VkDescriptorSetLayoutCreateInfo createInfo{
			    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			    .pNext = nullptr,
			    .flags = 0,
			    .bindingCount = static_cast<std::uint32_t>(descriptorBindings.size()),
			    .pBindings = descriptorBindings.data()};
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

	static void UpsertDescriptorBinding(std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings, VkDescriptorSetLayoutBinding binding) noexcept
	{
		for (VkDescriptorSetLayoutBinding& existingBinding : descriptorBindings)
		{
			if (existingBinding.binding == binding.binding && existingBinding.descriptorType == binding.descriptorType)
			{
				existingBinding.descriptorCount = std::max(existingBinding.descriptorCount, binding.descriptorCount);
				existingBinding.stageFlags |= binding.stageFlags;
				return;
			}
		}

		descriptorBindings.push_back(binding);
	}

	static VkDescriptorType ToVkDescriptorType(ShaderParameterSemanticKind semanticKind) noexcept
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
    std::vector<VkPushConstantRange> pushConstantRanges,
    std::vector<CompiledBinding> bindings,
    std::vector<std::string> bindingNames) noexcept :
	m_device(device), m_parameterLayout(&parameterLayout), m_descriptorSetLayouts(std::move(descriptorSetLayouts)),
	m_pushConstantRanges(std::move(pushConstantRanges)), m_bindings(std::move(bindings)), m_bindingNames(std::move(bindingNames))
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