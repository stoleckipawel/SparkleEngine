#include "../../PCH.h"
#include "Frame/Core/FrameRenderPath.h"

#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingCVars.h"

FrameRenderPath ResolveFrameRenderPathFromSettings() noexcept
{
	return CVarReferencePathTracingEnabled.Get()
	           ? FrameRenderPath::PathTracedReference
	           : FrameRenderPath::RealtimeDeferred;
}
