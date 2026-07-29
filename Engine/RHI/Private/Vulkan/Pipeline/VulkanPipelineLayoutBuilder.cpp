#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Pipeline/VulkanPipelineLayoutBuilder.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Diagnostics/VulkanDebugNames.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"

#include <format>

static const auto g_vulkanPipelineLayoutBuilderLogger = Logging::GetOrCreateLogger("RHI.Vulkan.PipelineLayout");

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, VkPipelineLayout layout) noexcept : m_device(device), m_layout(layout) {}

VulkanPipelineLayout::~VulkanPipelineLayout() noexcept
{
	Reset();
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept : m_device(other.m_device), m_layout(other.m_layout)
{
	other.m_device = VK_NULL_HANDLE;
	other.m_layout = VK_NULL_HANDLE;
}

VulkanPipelineLayout& VulkanPipelineLayout::operator=(VulkanPipelineLayout&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_device = other.m_device;
		m_layout = other.m_layout;
		other.m_device = VK_NULL_HANDLE;
		other.m_layout = VK_NULL_HANDLE;
	}
	return *this;
}

void VulkanPipelineLayout::Reset() noexcept
{
	if (m_device != VK_NULL_HANDLE && m_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_device, m_layout, nullptr);
	}
	m_layout = VK_NULL_HANDLE;
}

void VulkanPipelineLayoutBuilder::SetBindingLayout(const RenderBindingLayout* bindingLayout) noexcept
{
	m_descriptorSetLayouts = {};
	m_pushConstantRanges = {};
	if (bindingLayout == nullptr)
	{
		return;
	}

	const auto& vulkanBindingLayout = static_cast<const VulkanBindingLayout&>(*bindingLayout);
	m_descriptorSetLayouts = vulkanBindingLayout.GetDescriptorSetLayouts();
	m_pushConstantRanges = vulkanBindingLayout.GetPushConstantRanges();
}

std::unique_ptr<VulkanPipelineLayout> VulkanPipelineLayoutBuilder::Build(VulkanRhi& rhi, std::string_view debugName) const
{
	const VkPipelineLayoutCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .setLayoutCount = static_cast<std::uint32_t>(m_descriptorSetLayouts.size()),
	    .pSetLayouts = m_descriptorSetLayouts.data(),
	    .pushConstantRangeCount = static_cast<std::uint32_t>(m_pushConstantRanges.size()),
	    .pPushConstantRanges = m_pushConstantRanges.data()};

	VkPipelineLayout layout = VK_NULL_HANDLE;
	const VkResult result = vkCreatePipelineLayout(rhi.GetDevice(), &createInfo, nullptr, &layout);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fatal(
		    g_vulkanPipelineLayoutBuilderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("Failed to create Vulkan pipeline layout '{}': {}", debugName, VulkanResult::FormatFailure("vkCreatePipelineLayout", result)));
	}

	VulkanDebugNames::SetObjectName(
	    rhi.GetSetDebugUtilsObjectName(),
	    rhi.GetDevice(),
	    VK_OBJECT_TYPE_PIPELINE_LAYOUT,
	    reinterpret_cast<std::uint64_t>(layout),
	    std::format("{} Layout", debugName));

	return std::make_unique<VulkanPipelineLayout>(rhi.GetDevice(), layout);
}