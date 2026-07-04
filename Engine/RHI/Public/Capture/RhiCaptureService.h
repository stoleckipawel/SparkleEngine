#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Interop/ResourceState.h"
#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <filesystem>

enum class ERhiCaptureStatus : std::uint8_t
{
	Unsupported = 0,
	Failed = 1,
	Succeeded = 2
};

struct RhiTextureCaptureRequest final
{
	NativeResourceHandle Resource = {};
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	ResourceState SourceState = ResourceState::Common;
	std::filesystem::path OutputPath;
	std::uint32_t FrameIndex = 0;
	std::uint32_t ViewMode = 0;
	const char* ViewModeName = "";
	const char* DebugName = "";
};

struct RhiCaptureResult final
{
	ERhiCaptureStatus Status = ERhiCaptureStatus::Failed;
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::uint32_t FrameIndex = 0;
	std::uint32_t ViewMode = 0;
	const char* ViewModeName = "";
	std::filesystem::path ArtifactPath;
	const char* FailureReason = "";
};

class SPARKLE_RHI_API RhiCaptureService
{
  public:
	virtual ~RhiCaptureService() noexcept;

	virtual RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept = 0;
};
