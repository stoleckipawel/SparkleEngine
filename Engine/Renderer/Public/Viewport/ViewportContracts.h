#pragma once

#include "../RendererAPI.h"
#include "../Settings/EngineRenderingDisplayTypes.h"
#include "Core/Public/Hash/HashUtils.h"
#include "RenderViewMode.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class FramePipeline;
struct ViewportFrameProducts;

enum class RenderViewKind : std::uint8_t
{
	Game = 0,
	Scene = 1,
	Preview = 2,
	Thumbnail = 3,
	Debug = 4,
};

enum class RenderOutputFlags : std::uint16_t
{
	None = 0,
	FinalColorLdr = 1 << 0,
	SceneDepth = 1 << 1,
	ObjectId = 1 << 2,
	Normals = 1 << 3,
	OverlayMask = 1 << 4,
	Radiance = 1 << 5,
	RadianceSecondMoment = 1 << 6,
};

SPARKLE_RENDERER_API RenderOutputFlags operator|(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept;
SPARKLE_RENDERER_API RenderOutputFlags operator&(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept;
SPARKLE_RENDERER_API RenderOutputFlags& operator|=(RenderOutputFlags& lhs, RenderOutputFlags rhs) noexcept;
SPARKLE_RENDERER_API bool HasAnyRenderOutputFlags(RenderOutputFlags flags, RenderOutputFlags test) noexcept;

struct SPARKLE_RENDERER_API RenderViewportExtent
{
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;

	bool IsValid() const noexcept;
	bool operator==(const RenderViewportExtent& other) const noexcept;
};

struct SPARKLE_RENDERER_API ViewportExposureOverrides final
{
	bool OverrideMode = false;
	EngineExposureMode Mode = EngineExposureMode::Automatic;
	bool OverrideMeteringMethod = false;
	EngineExposureMeteringMethod MeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	bool OverrideManualExposure = false;
	float ManualExposure = 1.0f;
	bool OverrideCompensation = false;
	float Compensation = 0.0f;
	bool OverrideTargetLuminance = false;
	float TargetLuminance = 0.18f;
	bool OverrideMinimum = false;
	float Minimum = 0.000001f;
	bool OverrideMaximum = false;
	float Maximum = 65536.0f;
	bool OverrideAdaptationSpeedUp = false;
	float AdaptationSpeedUp = 3.0f;
	bool OverrideAdaptationSpeedDown = false;
	float AdaptationSpeedDown = 1.0f;

	bool operator==(const ViewportExposureOverrides&) const noexcept = default;
};

struct SPARKLE_RENDERER_API RenderViewSelectionToken
{
	std::uint64_t Value = 0;

	explicit operator bool() const noexcept;
};

struct SPARKLE_RENDERER_API RenderProductHandle
{
	std::uint64_t Value = 0;

	explicit operator bool() const noexcept;
};

enum class RenderProductFormat : std::uint8_t
{
	Unknown = 0,
	ColorLdr = 1,
	ColorHdr = 2,
	DepthStencil = 3,
	UnsignedInteger = 4,
	Float = 5,
};

struct RenderProductSamplePrefix final
{
	Hash::Sha256Digest RenderIdentitySha256 = {};
	std::uint64_t SampleCount = 0;
	std::uint64_t TargetSampleCount = 0;

	bool operator==(const RenderProductSamplePrefix&) const noexcept = default;
};

struct RenderProduct
{
	RenderProductHandle Handle = {};
	RenderViewportExtent Extent = {};
	RenderProductFormat Format = RenderProductFormat::Unknown;
	RenderProductSamplePrefix SamplePrefix = {};
};

struct SPARKLE_RENDERER_API ViewportCaptureRequest
{
	RenderOutputFlags Output = RenderOutputFlags::FinalColorLdr;
	// Zero accepts the currently published frame. A non-zero value rejects a
	// capture if the requested render product has already advanced.
	std::uint64_t ExpectedFrameId = 0;
};

enum class ViewportCaptureStatus : std::uint8_t
{
	Unavailable = 0,
	Failed = 1,
	Succeeded = 2,
};

struct SPARKLE_RENDERER_API ViewportCaptureResult
{
	ViewportCaptureStatus Status = ViewportCaptureStatus::Failed;
	std::uint64_t FrameId = 0;
	std::uint64_t SceneGeneration = 0;
	std::uint64_t ProviderGeneration = 0;
	RenderProductSamplePrefix SamplePrefix = {};
	std::string FailureReason;

	explicit operator bool() const noexcept;
};

struct SPARKLE_RENDERER_API ViewportCaptureId
{
	std::uint64_t Value = 0;

	explicit operator bool() const noexcept;
};

struct SPARKLE_RENDERER_API ViewportCaptureReadback
{
	ViewportCaptureResult Result;
	std::vector<std::byte> Pixels;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t RowPitch = 0;
	PixelFormat Format = PixelFormat::Unknown;
};

enum class ViewportRenderAction : std::uint8_t
{
	None = 0,
	Pause,
	Resume,
	Restart,
	Transfer,
};

struct SPARKLE_RENDERER_API ViewportRenderRequest
{
	std::uint64_t ViewportId = 0;
	std::uint64_t Generation = 0;
	std::uint64_t RenderActionSequence = 0;
	RenderViewKind ViewKind = RenderViewKind::Game;
	RenderViewMode ViewMode = RenderViewMode::Lit;
	ViewportRenderAction RenderAction = ViewportRenderAction::None;
	RenderViewportExtent Extent = {};
	RenderViewSelectionToken ViewSelection = {};
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::FinalColorLdr;
	ViewportExposureOverrides Exposure;
};

enum class ViewportRenderProgressState : std::uint8_t
{
	None = 0,
	Resetting,
	Accumulating,
	Paused,
	Unavailable,
	Complete,
};

enum class ViewportRenderProgressReason : std::uint8_t
{
	None = 0,
	ViewChanged,
	CameraChanged,
	GeometryChanged,
	DeformationChanged,
	MaterialChanged,
	LightingChanged,
	EnvironmentChanged,
	ShaderChanged,
	ExecutionChanged,
	BackendChanged,
	ManualRestart,
	Resumed,
	RetentionReleased,
	UnsupportedView,
	UnsupportedContent,
	UnsupportedCapability,
	SessionCapacity,
};

struct SPARKLE_RENDERER_API ViewportRenderProgress final
{
	ViewportRenderProgressState State = ViewportRenderProgressState::None;
	ViewportRenderProgressReason Reason = ViewportRenderProgressReason::None;
	std::uint64_t CompletedSamples = 0;
	std::uint64_t TargetSamples = 0;
	std::uint64_t DiscardedSamples = 0;
	std::uint64_t OwnerViewportId = 0;
	double SamplesPerSecond = 0.0;
	double EstimatedSecondsRemaining = 0.0;
	bool RetentionAvailable = false;
};

struct SPARKLE_RENDERER_API ViewportRenderProducts
{
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	RenderOutputFlags GetAvailableOutputs() const noexcept { return m_availableOutputs; }
	bool HasOutput(RenderOutputFlags output) const noexcept;

	const RenderProduct* FindProduct(RenderOutputFlags output) const noexcept;

	const RenderProduct& GetFinalColorLdr() const noexcept { return m_finalColorLdr; }
	const RenderProduct& GetSceneDepth() const noexcept { return m_sceneDepth; }
	const RenderProduct& GetObjectId() const noexcept { return m_objectId; }
	const RenderProduct& GetNormals() const noexcept { return m_normals; }
	const RenderProduct& GetOverlayMask() const noexcept { return m_overlayMask; }
	const ViewportRenderProgress& GetProgress() const noexcept { return m_progress; }

private:
	friend class FramePipeline;
	friend void PublishViewportRenderProducts(
	    ViewportRenderProducts& products,
	    const ViewportRenderRequest& request,
	    const ViewportFrameProducts& frameProducts,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent) noexcept;

	void Clear() noexcept;

	void SetGeneration(std::uint64_t generation) noexcept { m_generation = generation; }

	void ClearProduct(RenderOutputFlags output) noexcept;
	void SetProduct(RenderOutputFlags output, RenderProduct product) noexcept;
	void SetProgress(ViewportRenderProgress progress) noexcept { m_progress = progress; }

	RenderProduct* SelectProduct(RenderOutputFlags output) noexcept;
	const RenderProduct* SelectProduct(RenderOutputFlags output) const noexcept;
	void RemoveAvailableOutput(RenderOutputFlags output) noexcept;

	RenderOutputFlags m_availableOutputs = RenderOutputFlags::None;
	std::uint64_t m_generation = 0;
	RenderProduct m_finalColorLdr = {};
	RenderProduct m_sceneDepth = {};
	RenderProduct m_objectId = {};
	RenderProduct m_normals = {};
	RenderProduct m_overlayMask = {};
	RenderProduct m_radiance = {};
	RenderProduct m_radianceSecondMoment = {};
	ViewportRenderProgress m_progress = {};
};
