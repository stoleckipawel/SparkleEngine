#pragma once

#include "../Editor/EditorTextureHandle.h"
#include "../RendererAPI.h"

#include <cstdint>
#include <vector>

struct UiDrawVertex final
{
	float Position[2] = {};
	float Uv[2] = {};
	std::uint32_t Color = 0;
};

enum class UiDrawCommandKind : std::uint8_t
{
	Draw,
	ResetRenderState
};

struct UiDrawCommand final
{
	float ClipRect[4] = {};
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
	float DisplayPosition[2] = {};
	float DisplaySize[2] = {};
	float FramebufferScale[2] = {1.0f, 1.0f};
	std::vector<UiDrawVertex> Vertices;
	std::vector<std::uint32_t> Indices;
	std::vector<UiDrawCommand> Commands;
	std::vector<UiDrawList> DrawLists;

	bool HasDrawData() const noexcept;
};
