#pragma once

#include "Renderer/Public/RendererAPI.h"

class CommandContext;
class FrameGraph;
struct FrameContext;
struct RenderPassContext;

struct SPARKLE_RENDERER_API RenderGraphPassContext
{
	CommandContext& Commands;
	const FrameContext& Frame;
	const RenderPassContext& Runtime;
	const FrameGraph& Graph;
};