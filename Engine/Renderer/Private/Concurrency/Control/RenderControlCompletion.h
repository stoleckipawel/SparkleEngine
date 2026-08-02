#pragma once

#include "Diagnostics/MeshPreviewGeometry.h"
#include "Diagnostics/RendererMemoryDiagnostics.h"
#include "Meshes/MeshDiagnostics.h"
#include "Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <variant>

struct RenderControlError final
{
	std::string Message;
};

using RenderControlResult = std::variant<
    std::monostate,
    RenderControlError,
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
