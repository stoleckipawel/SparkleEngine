#pragma once

#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

class RenderCommandContext;
class PassExecutionDiagnostics;

struct PassCommandContext final
{
	RenderCommandContext& Commands;
	PassExecutionDiagnostics& Diagnostics;
	FrameGraphResourceCommands Resources;
};
