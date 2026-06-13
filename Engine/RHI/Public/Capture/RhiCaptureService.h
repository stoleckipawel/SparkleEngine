#pragma once

#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <filesystem>

struct RhiTextureCaptureRequest final
{
	NativeResourceHandle Resource = {};
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::filesystem::path OutputPath;
	const char* DebugName = "";
};

struct RhiCaptureResult final
{
	bool Succeeded = false;
	std::filesystem::path ArtifactPath;
	const char* FailureReason = "";

	constexpr explicit operator bool() const noexcept { return Succeeded; }
};

class SPARKLE_RHI_API RhiCaptureService
{
  public:
	virtual ~RhiCaptureService() noexcept = default;

	virtual RhiCaptureResult CaptureTextureToBmp(const RhiTextureCaptureRequest& request) noexcept = 0;
};
