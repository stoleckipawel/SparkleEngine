#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Textures/VulkanTextureLoader.h"

#include <format>

static const auto g_vulkanTextureLoaderLogger = Logging::GetOrCreateLogger("RHI.Vulkan.Textures");

std::unique_ptr<Texture> VulkanTextureLoader::Load(const std::filesystem::path& texturePath)
{
	Diagnostics::Fail(
	    g_vulkanTextureLoaderLogger,
	    __FILE__,
	    __LINE__,
	    std::format("Vulkan texture loading is not implemented yet: '{}'", texturePath.string()));
	return {};
}