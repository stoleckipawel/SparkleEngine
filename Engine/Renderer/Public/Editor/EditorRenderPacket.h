#pragma once

#include "../RendererAPI.h"

#include <cstdint>
#include <vector>

struct EditorDrawVertex final
{
	float Position[2] = {};
	float Uv[2] = {};
	std::uint32_t Color = 0;
};

enum class EditorDrawCommandKind : std::uint8_t
{
	Draw,
	ResetRenderState
};

struct EditorDrawCommand final
{
	float ClipRect[4] = {};
	std::uint64_t TextureHandle = 0;
	std::uint32_t ElementCount = 0;
	std::uint32_t IndexOffset = 0;
	std::int32_t VertexOffset = 0;
	EditorDrawCommandKind Kind = EditorDrawCommandKind::Draw;
};

struct EditorDrawList final
{
	std::uint32_t VertexOffset = 0;
	std::uint32_t VertexCount = 0;
	std::uint32_t IndexOffset = 0;
	std::uint32_t IndexCount = 0;
	std::uint32_t CommandOffset = 0;
	std::uint32_t CommandCount = 0;
};

struct SPARKLE_RENDERER_API EditorRenderPacket final
{
	std::uint64_t UiFrameId = 0;
	std::uint64_t ViewportGeneration = 0;
	float DisplayPosition[2] = {};
	float DisplaySize[2] = {};
	float FramebufferScale[2] = {1.0f, 1.0f};
	std::vector<EditorDrawVertex> Vertices;
	std::vector<std::uint32_t> Indices;
	std::vector<EditorDrawCommand> Commands;
	std::vector<EditorDrawList> DrawLists;

	bool HasDrawData() const noexcept;
};
