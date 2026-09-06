#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/ResourceState.h"
#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

enum class ERhiCaptureStatus : std::uint8_t
{
	Unsupported = 0,
	Failed = 1,
	Succeeded = 2
};

struct RhiCaptureTicket final
{
	std::uint64_t Value = 0;

	explicit operator bool() const noexcept;
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
	std::string ViewModeName;
	std::string DebugName;
};

struct RhiCaptureResult final
{
	ERhiCaptureStatus Status = ERhiCaptureStatus::Failed;
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::uint64_t FrameId = 0;
	std::uint32_t ViewMode = 0;
	std::string ViewModeName;
	std::filesystem::path ArtifactPath;
	std::string FailureReason;
};

struct RhiCaptureReadback final
{
	RhiCaptureResult Result;
	std::vector<std::byte> Pixels;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t RowPitch = 0;
	PixelFormat Format = PixelFormat::Unknown;
};

class SPARKLE_RHI_API RhiCaptureService
{
public:
	virtual ~RhiCaptureService() noexcept;
	RhiCaptureService(const RhiCaptureService&) = delete;
	RhiCaptureService& operator=(const RhiCaptureService&) = delete;
	RhiCaptureService(RhiCaptureService&&) = delete;
	RhiCaptureService& operator=(RhiCaptureService&&) = delete;

	virtual RhiCaptureTicket BeginTextureReadback(const RhiTextureCaptureRequest& request) noexcept = 0;
	virtual bool TryTakeTextureReadback(RhiCaptureTicket ticket, RhiCaptureReadback& readback) noexcept = 0;
	virtual void CancelTextureReadback(RhiCaptureTicket ticket) noexcept = 0;

protected:
	RhiCaptureService() noexcept = default;
};

SPARKLE_RHI_API bool WriteRhiCaptureBmp(const RhiCaptureReadback& readback, const std::filesystem::path& outputPath) noexcept;
