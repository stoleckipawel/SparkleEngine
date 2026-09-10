#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "View/RenderView.h"

ViewportRenderProgress ReferencePathTracer::Update(const RenderView& view) const noexcept
{
	if (view.viewMode != RenderViewMode::ReferencePathTracer)
	{
		return {};
	}

	return ViewportRenderProgress{
	    .ViewMode = RenderViewMode::ReferencePathTracer,
	    .State = ViewportRenderProgressState::Unavailable,
	    .CompletedWork = 0,
	    .TargetWork = 0};
}
