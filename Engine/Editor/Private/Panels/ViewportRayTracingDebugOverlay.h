#pragma once

#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"

#include <imgui.h>

class ViewportRayTracingDebugOverlay final
{
  public:
	static void Draw(const RendererSmokeDiagnosticsSnapshot& diagnostics, const ImVec2& viewportMin) noexcept;
};
