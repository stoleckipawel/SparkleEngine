#include "D3D12/Capture/D3D12CaptureService.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

D3D12CaptureService::D3D12CaptureService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiCaptureResult D3D12CaptureService::CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept
{
	const bool captured =
	    m_owner != nullptr && m_owner->CaptureTextureToBmp(request.Resource, request.Width, request.Height, request.OutputPath);
	return RhiCaptureResult{
	    .Status = captured ? ERhiCaptureStatus::Succeeded : ERhiCaptureStatus::Failed,
	    .BackendApi = ERhiBackendApi::D3D12,
	    .FrameIndex = request.FrameIndex,
	    .ViewMode = request.ViewMode,
	    .ViewModeName = request.ViewModeName,
	    .ArtifactPath = captured ? request.OutputPath : std::filesystem::path{},
	    .FailureReason = captured ? "" : "D3D12 texture capture failed; verify the resource is a valid Texture2D and the output path is writable."};
}
