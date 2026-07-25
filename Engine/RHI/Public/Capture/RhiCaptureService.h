#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/ResourceState.h"
#include "../Resources/RhiResourceHandles.h"
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
	RhiResourceHandle Resource = {};
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	PixelFormat SourceFormat = PixelFormat::Unknown;
	ResourceState SourceState = ResourceState::Common;
	std::filesystem::path OutputPath;
	std::uint64_t FrameId = 0;
	std::uint32_t ViewMode = 0;
	const char* ViewModeName = "";
	const char* DebugName = "";
};

struct RhiCaptureResult final
{
	ERhiCaptureStatus Status = ERhiCaptureStatus::Failed;
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::uint64_t FrameId = 0;
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
