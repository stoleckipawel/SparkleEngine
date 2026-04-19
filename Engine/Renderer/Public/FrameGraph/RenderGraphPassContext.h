#pragma once

#include "../RendererAPI.h"

class CommandContext;
class FrameGraph;
class PassExecutionDiagnostics;
struct FrameContext;
struct RenderPassContext;

struct SPARKLE_RENDERER_API RenderGraphPassContext
{
	CommandContext& Commands;
	const FrameContext& Frame;
	const RenderPassContext& Runtime;
	PassExecutionDiagnostics& Diagnostics;
	const FrameGraph& Graph;
};