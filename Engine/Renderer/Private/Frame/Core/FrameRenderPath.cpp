#include "../../PCH.h"
#include "Frame/Core/FrameRenderPath.h"

#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingSettings.h"

FrameRenderPath ResolveFrameRenderPathFromSettings() noexcept
{
	return BuildReferencePathTracingSettingsFromCVars().Enabled
	           ? FrameRenderPath::PathTracedReference
	           : FrameRenderPath::RealtimeDeferred;
}
