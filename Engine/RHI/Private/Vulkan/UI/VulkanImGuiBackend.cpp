#include "Vulkan/VulkanPCH.h"

#include "Vulkan/UI/VulkanImGuiBackend.h"

static const auto g_vulkanImGuiBackendLogger = Logging::GetOrCreateLogger("RHI.Vulkan.ImGui");

bool VulkanImGuiBackend::Initialize()
{
	SPDLOG_LOGGER_WARN(g_vulkanImGuiBackendLogger, "Vulkan ImGui backend is not implemented yet.");
	return false;
}

void VulkanImGuiBackend::BeginFrame() noexcept {}

void VulkanImGuiBackend::RenderDrawData(ImDrawData*) noexcept {}

void VulkanImGuiBackend::Shutdown() noexcept {}