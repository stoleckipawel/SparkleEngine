#include "Vulkan/Capture/VulkanCaptureService.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanCaptureService::VulkanCaptureService(VulkanRenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiCaptureResult VulkanCaptureService::CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept
{
	const bool captured =
	    m_owner != nullptr && m_owner->CaptureTextureToBmp(request.Resource, request.Width, request.Height, request.OutputPath);
	return RhiCaptureResult{
	    .Status = captured ? ERhiCaptureStatus::Succeeded : ERhiCaptureStatus::Failed,
	    .BackendApi = ERhiBackendApi::Vulkan,
	    .FrameIndex = request.FrameIndex,
	    .ViewMode = request.ViewMode,
	    .ViewModeName = request.ViewModeName,
	    .ArtifactPath = captured ? request.OutputPath : std::filesystem::path{},
	    .FailureReason = captured ? "" : "Vulkan texture capture failed; verify the resource is a valid VkImage and the output path is writable."};
}
