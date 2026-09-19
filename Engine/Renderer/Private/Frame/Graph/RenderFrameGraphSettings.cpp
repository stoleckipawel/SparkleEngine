#include "../../PCH.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"

#include "RayReconstruction/RayReconstructionSettings.h"

bool ShouldUseRayReconstruction(RenderViewMode viewMode) noexcept
{
	return viewMode == RenderViewMode::Lit && IsRayReconstructionEnabled();
}
