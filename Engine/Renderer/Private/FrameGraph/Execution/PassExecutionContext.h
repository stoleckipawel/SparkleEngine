#pragma once

#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

class RenderCommandContext;
class PassExecutionDiagnostics;
struct FrameContext;
struct PassRuntimeServices;

struct PassExecutionContext
{
	RenderCommandContext& Commands;
	const FrameContext& Frame;
	const PassRuntimeServices& RuntimeServices;
	PassExecutionDiagnostics& Diagnostics;
	FrameGraphResourceCommands Resources;
};