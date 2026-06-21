#pragma once

#include "Renderer/Public/Diagnostics/RendererDiagnosticsSnapshot.h"

class RendererSystemRoot;

class RendererDiagnosticsCollector final
{
  public:
	static RendererDiagnosticsSnapshot Capture(const RendererSystemRoot& systems);
};
