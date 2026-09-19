#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactActions.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactCoordinator.h"
#include "Editor/Public/Panels/ViewportOutputAction.h"

void ApplyReferencePathTracerArtifactAction(
    ViewportOutputAction action,
    ReferencePathTracerArtifactCoordinator& artifacts,
    Renderer& renderer,
    const ViewportRenderProducts& products)
{
	switch (action)
	{
		case ViewportOutputAction::SaveCurrentPrefix:
			artifacts.Request(ReferencePathTracerArtifactKind::PartialPrefix, renderer, products);
			break;
		case ViewportOutputAction::SaveComplete:
			artifacts.Request(ReferencePathTracerArtifactKind::Complete, renderer, products);
			break;
		case ViewportOutputAction::SaveCheckpoint:
			artifacts.Request(ReferencePathTracerArtifactKind::Checkpoint, renderer, products);
			break;
		case ViewportOutputAction::CapturePresentation:
		case ViewportOutputAction::None:
			break;
	}
}
