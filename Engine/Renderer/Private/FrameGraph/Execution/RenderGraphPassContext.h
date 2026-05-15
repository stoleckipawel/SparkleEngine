#pragma once

class RenderCommandContext;
class FrameGraph;
class PassExecutionDiagnostics;
struct FrameContext;
struct RenderPassContext;

struct RenderGraphPassContext
{
	RenderCommandContext& Commands;
	const FrameContext& Frame;
	const RenderPassContext& Runtime;
	PassExecutionDiagnostics& Diagnostics;
	const FrameGraph& Graph;
};