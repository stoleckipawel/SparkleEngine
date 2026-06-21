#pragma once

#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"

class FramePipeline;
class RendererSystemRoot;

namespace RendererSmokeDiagnosticsBuilder
{
	RendererSmokeDiagnosticsSnapshot Build(const RendererSystemRoot& systems, const FramePipeline* framePipeline);
}
