#pragma once

#include "Renderer/Public/RendererAPI.h"

class RenderCommandContext;
class FrameGraph;
class PassExecutionDiagnostics;
struct FrameContext;
struct RenderPassContext;

struct SPARKLE_RENDERER_API RenderGraphPassContext
{
	RenderCommandContext& Commands;
	const FrameContext& Frame;
	const RenderPassContext& Runtime;
	PassExecutionDiagnostics& Diagnostics;
	const FrameGraph& Graph;
};