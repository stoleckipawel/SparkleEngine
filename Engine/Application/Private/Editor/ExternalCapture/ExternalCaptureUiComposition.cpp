#include "PCH.h"
#include "Editor/ExternalCapture/ExternalCaptureUiComposition.h"
#include "Editor/Public/ExternalCapture/ExternalCaptureOverlay.h"
#include "Renderer.h"

class RendererCaptureCommands final : public EditorExternalCaptureCommands
{
public:
	explicit RendererCaptureCommands(Renderer& renderer) noexcept :

	    m_renderer(renderer)
	{
	}

	ExternalCaptureSnapshot Observe() const override { return m_renderer.ObserveExternalCapture(); }

	ExternalCaptureAdmission Request() noexcept override
	{
		const auto presentation = m_renderer.GetViewportPresentation();
		return m_renderer.RequestExternalCapture(presentation.Products.GetGeneration());
	}

private:
	Renderer& m_renderer;
};

std::unique_ptr<ViewportOverlay> CreateEditorExternalCaptureOverlay(Renderer& renderer)
{
	return CreateExternalCaptureOverlay(std::make_unique<RendererCaptureCommands>(renderer));
}
