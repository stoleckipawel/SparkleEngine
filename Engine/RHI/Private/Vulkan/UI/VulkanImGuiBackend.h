#pragma once

#include "UI/RhiImGuiRenderer.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <vector>

struct ImDrawData;
class VulkanRenderHardwareInterface;

class VulkanImGuiBackend final : public RhiImGuiRenderer
{
  public:
	explicit VulkanImGuiBackend(VulkanRenderHardwareInterface& renderHardware) noexcept;

	bool Initialize() override;
	void BeginFrame() noexcept override;
	void RenderDrawData(ImDrawData* drawData) noexcept override;
	void Shutdown() noexcept override;
	std::uint64_t GetTextureId(VkImageView imageView) noexcept;

  private:
	struct TextureBinding final
	{
		VkImageView ImageView = VK_NULL_HANDLE;
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
	};

	VulkanRenderHardwareInterface* m_renderHardware = nullptr;
	VkSampler m_imguiSampler = VK_NULL_HANDLE;
	std::vector<TextureBinding> m_textureBindings;
};
