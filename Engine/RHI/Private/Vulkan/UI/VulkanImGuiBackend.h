#pragma once

#include "UI/RhiImGuiRenderer.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <vector>

struct ImDrawData;
class VulkanDescriptorManager;
class VulkanRenderHardwareInterface;

class VulkanImGuiBackend final : public RhiImGuiRenderer
{
  public:
	VulkanImGuiBackend(VulkanRenderHardwareInterface& renderHardware, VulkanDescriptorManager& descriptorManager) noexcept;

	bool Initialize() override;
	void BeginFrame() noexcept override;
	std::uint64_t ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	void RenderDrawData(ImDrawData* drawData) noexcept override;
	void Shutdown() noexcept override;

  private:
	struct TextureBinding final
	{
		VkImageView ImageView = VK_NULL_HANDLE;
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
	};

	std::uint64_t GetTextureId(VkImageView imageView) noexcept;

	VulkanRenderHardwareInterface* m_renderHardware = nullptr;
	VulkanDescriptorManager* m_descriptorManager = nullptr;
	VkSampler m_imguiSampler = VK_NULL_HANDLE;
	std::vector<TextureBinding> m_textureBindings;
};
