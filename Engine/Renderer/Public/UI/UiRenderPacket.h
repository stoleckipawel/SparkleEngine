#pragma once

#include "../Editor/EditorTextureHandle.h"
#include "../RendererAPI.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct UiDrawVertex final
{
	std::array<float, 2> Position = {};
	std::array<float, 2> Uv = {};
	std::uint32_t Color = 0;
};

enum class UiDrawCommandKind : std::uint8_t
{
	Draw,
	ResetRenderState
};

struct UiDrawCommand final
{
	std::array<float, 4> ClipRect = {};
	EditorTextureHandle TextureHandle = {};
	std::uint32_t ElementCount = 0;
	std::uint32_t IndexOffset = 0;
	std::int32_t VertexOffset = 0;
	UiDrawCommandKind Kind = UiDrawCommandKind::Draw;
};

struct UiDrawList final
{
	std::uint32_t VertexOffset = 0;
	std::uint32_t VertexCount = 0;
	std::uint32_t IndexOffset = 0;
	std::uint32_t IndexCount = 0;
	std::uint32_t CommandOffset = 0;
	std::uint32_t CommandCount = 0;
};

struct UiTextureUpload final
{
	EditorTextureHandle Texture;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t PixelOffset = 0;
	std::uint32_t PixelCount = 0;
};

enum class UiPresentationMode : std::uint8_t
{
	None,
	EditorViewport,
	HostOverlay
};

struct SPARKLE_RENDERER_API UiRenderPacket final
{
	std::uint64_t UiFrameId = 0;
	std::uint64_t ViewportGeneration = 0;
	UiPresentationMode PresentationMode = UiPresentationMode::None;
	std::array<float, 2> DisplayPosition = {};
	std::array<float, 2> DisplaySize = {};
	std::array<float, 2> FramebufferScale = {1.0f, 1.0f};
	std::vector<UiDrawVertex> Vertices;
	std::vector<std::uint32_t> Indices;
	std::vector<UiDrawCommand> Commands;
	std::vector<UiDrawList> DrawLists;
	std::vector<std::byte> TexturePixels;
	std::vector<UiTextureUpload> TextureUploads;
	std::vector<EditorTextureHandle> TextureReleases;

	bool HasDrawData() const noexcept;
};
