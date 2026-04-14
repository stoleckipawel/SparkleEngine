#pragma once

#include "../RendererAPI.h"

#include <cstdint>

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
	return static_cast<RenderFeatureFlags>(
	    static_cast<std::uint16_t>(lhs) |
	    static_cast<std::uint16_t>(rhs));
}

constexpr RenderFeatureFlags operator&(RenderFeatureFlags lhs, RenderFeatureFlags rhs) noexcept
{
	return static_cast<RenderFeatureFlags>(
	    static_cast<std::uint16_t>(lhs) &
	    static_cast<std::uint16_t>(rhs));
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
	return static_cast<RenderOutputFlags>(
	    static_cast<std::uint16_t>(lhs) |
	    static_cast<std::uint16_t>(rhs));
}

constexpr RenderOutputFlags operator&(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept
{
	return static_cast<RenderOutputFlags>(
	    static_cast<std::uint16_t>(lhs) &
	    static_cast<std::uint16_t>(rhs));
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

	constexpr bool IsValid() const noexcept
	{
		return Width > 0 && Height > 0;
	}
};

struct RenderViewSelectionToken
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept
	{
		return Value != 0;
	}
};

struct RenderProductHandle
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept
	{
		return Value != 0;
	}
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
	RenderOutputFlags AvailableOutputs = RenderOutputFlags::None;
	RenderProduct SceneColor = {};
	RenderProduct SceneDepth = {};
	RenderProduct ObjectId = {};
	RenderProduct Normals = {};
	RenderProduct OverlayMask = {};
};