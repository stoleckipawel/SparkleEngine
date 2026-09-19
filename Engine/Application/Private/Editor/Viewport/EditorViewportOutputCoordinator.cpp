#include "PCH.h"

#include "Editor/Viewport/EditorViewportOutputCoordinator.h"

#include "Editor/Capture/EditorViewportCaptureCoordinator.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactActions.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactCoordinator.h"
#include "Editor/Public/UI.h"
#include "Renderer.h"

struct EditorViewportOutputCoordinator::State final
{
	explicit State(EditorOperationRuntime& operations) noexcept :
	    PresentationCapture(operations),
	    ReferenceArtifacts(operations)
	{
	}

	EditorViewportCaptureCoordinator PresentationCapture;
	ReferencePathTracerArtifactCoordinator ReferenceArtifacts;
};

EditorViewportOutputCoordinator::EditorViewportOutputCoordinator(EditorOperationRuntime& operations) noexcept :
    m_state(std::make_unique<State>(operations))
{
}

EditorViewportOutputCoordinator::~EditorViewportOutputCoordinator() noexcept = default;

void EditorViewportOutputCoordinator::Update(Renderer& renderer)
{
	m_state->PresentationCapture.Update(renderer);
	m_state->ReferenceArtifacts.Update(renderer, renderer.GetViewportRenderProducts());
}

void EditorViewportOutputCoordinator::HandleAction(UI& ui, Renderer& renderer, std::uint64_t frameId)
{
	const ViewportOutputAction action = ui.ConsumeViewportOutputAction();
	if (action == ViewportOutputAction::CapturePresentation)
	{
		m_state->PresentationCapture.Request(renderer, frameId);
		return;
	}
	ApplyReferencePathTracerArtifactAction(action, m_state->ReferenceArtifacts, renderer, renderer.GetViewportRenderProducts());
}
