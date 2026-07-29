#pragma once

#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

class RenderCommandContext;
class PassExecutionDiagnostics;
struct FrameContext;
struct PassRuntimeContext;

struct PassExecutionContext
{
	RenderCommandContext& Commands;
	const FrameContext& Frame;
	const PassRuntimeContext& Runtime;
	PassExecutionDiagnostics& Diagnostics;
	FrameGraphResourceCommands Resources;
};
