#pragma once

#include "UI/RhiImGuiRenderer.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <vector>

struct ImDrawData;
struct ImGuiContext;
class VulkanDescriptorService;
class VulkanRenderHardwareInterface;

class VulkanImGuiBackend final : public RhiImGuiRenderer
{
  public:
	VulkanImGuiBackend(VulkanRenderHardwareInterface& renderHardwareInterface, VulkanDescriptorService& descriptorService) noexcept;

	void Initialize() override;
	void BeginFrame() noexcept override;
	std::uint64_t ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	void RenderDrawData(ImDrawData* drawData) noexcept override;
	void ReleaseTexture(ImTextureData& texture) noexcept override;
	void Shutdown() noexcept override;

  private:
	struct TextureBinding final
	{
		VkImageView ImageView = VK_NULL_HANDLE;
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
	};

	std::uint64_t GetTextureId(VkImageView imageView) noexcept;
	ImGuiContext* ActivateContext() const noexcept;
	static void RestoreContext(ImGuiContext* context) noexcept;

	VulkanRenderHardwareInterface& m_renderHardwareInterface;
	VulkanDescriptorService& m_descriptorService;
	VkSampler m_imguiSampler = VK_NULL_HANDLE;
	std::vector<TextureBinding> m_textureBindings;
	ImGuiContext* m_imguiContext = nullptr;
	bool m_ownsContext = false;
};
