#include "PCH.h"

#include "Renderer/Public/Debug/RendererCVars.h"

ConsoleVariable<RenderViewMode> CVarRenderViewMode("r.ViewMode", RenderViewMode::Lit, "Renderer debug view mode.");
ConsoleVariable<bool> CVarRendererMeshAutoBatching(
	"r.MeshAutoBatching",
	true,
	"Build renderer-side auto batches for compatible flat mesh instances.");