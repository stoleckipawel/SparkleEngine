#pragma once

#include "Diagnostics/MeshPreviewGeometry.h"
#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Shaders/CookedShaderReloadResult.h"
#include "Viewport/ViewportContracts.h"

#include <condition_variable>
#include <mutex>
#include <variant>

using RenderControlResult = std::variant<
    std::monostate,
    CookedShaderReloadResult,
    MeshDiagnosticsSnapshot,
    MeshPreviewGeometry,
    TextureDiagnosticsSnapshot,
    RendererMemoryDiagnosticsSnapshot>;

class RenderControlCompletion final
{
  public:
	void Complete(RenderControlResult result);
	void Cancel();
	RenderControlResult Wait();

  private:
	std::mutex m_mutex;
	std::condition_variable m_completedCondition;
	RenderControlResult m_result;
	bool m_completed = false;
};
