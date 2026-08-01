#pragma once

#include "../Editor/EditorTextureHandle.h"
#include "../RendererAPI.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

enum class RenderViewKind : std::uint8_t
{
	Game = 0,
	Scene = 1,
	Preview = 2,
	Thumbnail = 3,
	Debug = 4,
};

enum class RenderFeatureFlags : std::uint16_t
{
	None = 0,
	Picking = 1 << 0,
	Wireframe = 1 << 1,
	LightingOnly = 1 << 2,
	DebugOverlay = 1 << 3,
	GizmoOverlay = 1 << 4,
};

SPARKLE_RENDERER_API RenderFeatureFlags operator|(RenderFeatureFlags lhs, RenderFeatureFlags rhs) noexcept;
SPARKLE_RENDERER_API RenderFeatureFlags operator&(RenderFeatureFlags lhs, RenderFeatureFlags rhs) noexcept;
SPARKLE_RENDERER_API RenderFeatureFlags& operator|=(RenderFeatureFlags& lhs, RenderFeatureFlags rhs) noexcept;
SPARKLE_RENDERER_API bool HasAnyRenderFeatureFlags(RenderFeatureFlags flags, RenderFeatureFlags test) noexcept;

enum class RenderOutputFlags : std::uint16_t
{
	None = 0,
	SceneColor = 1 << 0,
	SceneDepth = 1 << 1,
	ObjectId = 1 << 2,
	Normals = 1 << 3,
	OverlayMask = 1 << 4,
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

struct RenderProduct
{
	RenderProductHandle Handle = {};
	RenderViewportExtent Extent = {};
	RenderProductFormat Format = RenderProductFormat::Unknown;
	EditorTextureHandle EditorTexture = {};
};

struct SPARKLE_RENDERER_API ViewportCaptureRequest
{
	RenderOutputFlags Output = RenderOutputFlags::SceneColor;
	std::filesystem::path OutputPath;
	// Zero accepts the currently published frame. A non-zero value rejects a
	// capture if the requested render product has already advanced.
	std::uint64_t ExpectedFrameId = 0;
	std::uint32_t ViewMode = 0;
	std::string ViewModeName;
	std::string DebugName;
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
	std::filesystem::path ArtifactPath;
	std::string FailureReason;

	explicit operator bool() const noexcept;
};

struct SPARKLE_RENDERER_API ViewportCaptureId
{
	std::uint64_t Value = 0;

	explicit operator bool() const noexcept;
};

enum class ViewportCapturePixelFormat : std::uint8_t
{
	Rgba32Float = 0,
	Rgba16Float,
	Rgba8Unorm,
	Bgra8Unorm,
};

struct SPARKLE_RENDERER_API ViewportCaptureReadback
{
	ViewportCaptureId Id;
	ViewportCaptureResult Result;
	std::vector<std::byte> Pixels;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t RowPitch = 0;
	ViewportCapturePixelFormat Format =
	    ViewportCapturePixelFormat::Rgba8Unorm;
};

SPARKLE_RENDERER_API bool WriteViewportCaptureBmp(
    const ViewportCaptureReadback& readback) noexcept;

struct SPARKLE_RENDERER_API ViewportRenderRequest
{
	std::uint64_t ViewportId = 0;
	std::uint64_t Generation = 0;
	RenderViewKind ViewKind = RenderViewKind::Game;
	RenderViewportExtent Extent = {};
	RenderViewSelectionToken ViewSelection = {};
	RenderFeatureFlags FeatureFlags = RenderFeatureFlags::None;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::SceneColor;
};

struct SPARKLE_RENDERER_API ViewportRenderProducts
{
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	RenderOutputFlags GetAvailableOutputs() const noexcept { return m_availableOutputs; }
	bool HasOutput(RenderOutputFlags output) const noexcept;

	const RenderProduct* FindProduct(RenderOutputFlags output) const noexcept;

	const RenderProduct& GetSceneColor() const noexcept { return m_sceneColor; }
	const RenderProduct& GetSceneDepth() const noexcept { return m_sceneDepth; }
	const RenderProduct& GetObjectId() const noexcept { return m_objectId; }
	const RenderProduct& GetNormals() const noexcept { return m_normals; }
	const RenderProduct& GetOverlayMask() const noexcept { return m_overlayMask; }

	void Clear() noexcept;

	void SetGeneration(std::uint64_t generation) noexcept { m_generation = generation; }

	void ClearProduct(RenderOutputFlags output) noexcept;
	void SetProduct(RenderOutputFlags output, RenderProduct product) noexcept;

  private:
	RenderProduct* SelectProduct(RenderOutputFlags output) noexcept;
	const RenderProduct* SelectProduct(RenderOutputFlags output) const noexcept;
	void RemoveAvailableOutput(RenderOutputFlags output) noexcept;

	RenderOutputFlags m_availableOutputs = RenderOutputFlags::None;
	std::uint64_t m_generation = 0;
	RenderProduct m_sceneColor = {};
	RenderProduct m_sceneDepth = {};
	RenderProduct m_objectId = {};
	RenderProduct m_normals = {};
	RenderProduct m_overlayMask = {};
};
