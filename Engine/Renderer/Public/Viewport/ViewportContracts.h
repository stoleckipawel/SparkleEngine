#pragma once

#include "../RendererAPI.h"

#include <cstdint>
#include <filesystem>

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

constexpr RenderFeatureFlags operator|(RenderFeatureFlags lhs, RenderFeatureFlags rhs) noexcept
{
	return static_cast<RenderFeatureFlags>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

constexpr RenderFeatureFlags operator&(RenderFeatureFlags lhs, RenderFeatureFlags rhs) noexcept
{
	return static_cast<RenderFeatureFlags>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}

constexpr RenderFeatureFlags& operator|=(RenderFeatureFlags& lhs, RenderFeatureFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyRenderFeatureFlags(RenderFeatureFlags flags, RenderFeatureFlags test) noexcept
{
	return (flags & test) != RenderFeatureFlags::None;
}

enum class RenderOutputFlags : std::uint16_t
{
	None = 0,
	SceneColor = 1 << 0,
	SceneDepth = 1 << 1,
	ObjectId = 1 << 2,
	Normals = 1 << 3,
	OverlayMask = 1 << 4,
};

constexpr RenderOutputFlags operator|(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept
{
	return static_cast<RenderOutputFlags>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

constexpr RenderOutputFlags operator&(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept
{
	return static_cast<RenderOutputFlags>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}

constexpr RenderOutputFlags& operator|=(RenderOutputFlags& lhs, RenderOutputFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyRenderOutputFlags(RenderOutputFlags flags, RenderOutputFlags test) noexcept
{
	return (flags & test) != RenderOutputFlags::None;
}

struct RenderViewportExtent
{
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;

	constexpr bool IsValid() const noexcept { return Width > 0 && Height > 0; }
};

struct RenderViewSelectionToken
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

struct RenderProductHandle
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
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
};

enum class ViewportPresentationStatus : std::uint8_t
{
	Unavailable = 0,
	Ready = 1,
};

struct SPARKLE_RENDERER_API ViewportPresentationProduct
{
	RenderOutputFlags Output = RenderOutputFlags::None;
	RenderProduct Product = {};
	std::uint64_t TextureId = 0;
	ViewportPresentationStatus Status = ViewportPresentationStatus::Unavailable;
	const char* FailureReason = "";

	constexpr explicit operator bool() const noexcept
	{
		return Status == ViewportPresentationStatus::Ready && TextureId != 0;
	}
};

struct SPARKLE_RENDERER_API ViewportCaptureRequest
{
	RenderOutputFlags Output = RenderOutputFlags::SceneColor;
	std::filesystem::path OutputPath;
	std::uint32_t FrameIndex = 0;
	std::uint32_t ViewMode = 0;
	const char* ViewModeName = "";
	const char* DebugName = "";
};

struct SPARKLE_RENDERER_API ViewportRenderRequest
{
	std::uint64_t ViewportId = 0;
	RenderViewKind ViewKind = RenderViewKind::Game;
	RenderViewportExtent Extent = {};
	RenderViewSelectionToken ViewSelection = {};
	RenderFeatureFlags FeatureFlags = RenderFeatureFlags::None;
	RenderOutputFlags RequestedOutputs = RenderOutputFlags::SceneColor;
};

struct SPARKLE_RENDERER_API ViewportRenderProducts
{
	RenderOutputFlags GetAvailableOutputs() const noexcept { return m_availableOutputs; }
	bool HasOutput(RenderOutputFlags output) const noexcept { return HasAnyRenderOutputFlags(m_availableOutputs, output); }

	const RenderProduct* FindProduct(RenderOutputFlags output) const noexcept
	{
		const RenderProduct* product = SelectProduct(output);
		return product != nullptr && HasOutput(output) ? product : nullptr;
	}

	const RenderProduct& GetSceneColor() const noexcept { return m_sceneColor; }
	const RenderProduct& GetSceneDepth() const noexcept { return m_sceneDepth; }
	const RenderProduct& GetObjectId() const noexcept { return m_objectId; }
	const RenderProduct& GetNormals() const noexcept { return m_normals; }
	const RenderProduct& GetOverlayMask() const noexcept { return m_overlayMask; }

	void Clear() noexcept
	{
		m_availableOutputs = RenderOutputFlags::None;
		m_sceneColor = {};
		m_sceneDepth = {};
		m_objectId = {};
		m_normals = {};
		m_overlayMask = {};
	}

	void ClearProduct(RenderOutputFlags output) noexcept
	{
		RenderProduct* product = SelectProduct(output);
		if (product == nullptr)
		{
			return;
		}

		*product = {};
		RemoveAvailableOutput(output);
	}

	void SetProduct(RenderOutputFlags output, RenderProduct product) noexcept
	{
		RenderProduct* target = SelectProduct(output);
		if (target == nullptr)
		{
			return;
		}

		*target = product;
		if (product.Handle)
		{
			m_availableOutputs |= output;
		}
		else
		{
			RemoveAvailableOutput(output);
		}
	}

  private:
	RenderProduct* SelectProduct(RenderOutputFlags output) noexcept
	{
		switch (output)
		{
			case RenderOutputFlags::SceneColor:
				return &m_sceneColor;
			case RenderOutputFlags::SceneDepth:
				return &m_sceneDepth;
			case RenderOutputFlags::ObjectId:
				return &m_objectId;
			case RenderOutputFlags::Normals:
				return &m_normals;
			case RenderOutputFlags::OverlayMask:
				return &m_overlayMask;
			case RenderOutputFlags::None:
			default:
				return nullptr;
		}
	}

	const RenderProduct* SelectProduct(RenderOutputFlags output) const noexcept
	{
		switch (output)
		{
			case RenderOutputFlags::SceneColor:
				return &m_sceneColor;
			case RenderOutputFlags::SceneDepth:
				return &m_sceneDepth;
			case RenderOutputFlags::ObjectId:
				return &m_objectId;
			case RenderOutputFlags::Normals:
				return &m_normals;
			case RenderOutputFlags::OverlayMask:
				return &m_overlayMask;
			case RenderOutputFlags::None:
			default:
				return nullptr;
		}
	}

	void RemoveAvailableOutput(RenderOutputFlags output) noexcept
	{
		m_availableOutputs = static_cast<RenderOutputFlags>(
		    static_cast<std::uint16_t>(m_availableOutputs) & ~static_cast<std::uint16_t>(output));
	}

	RenderOutputFlags m_availableOutputs = RenderOutputFlags::None;
	RenderProduct m_sceneColor = {};
	RenderProduct m_sceneDepth = {};
	RenderProduct m_objectId = {};
	RenderProduct m_normals = {};
	RenderProduct m_overlayMask = {};
};
