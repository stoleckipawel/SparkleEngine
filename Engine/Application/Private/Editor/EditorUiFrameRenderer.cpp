#include "PCH.h"

#include "Editor/EditorUiFrameRenderer.h"

#include "Editor/Public/UI.h"
#include "Renderer.h"

void EditorUiFrameRenderer::Render(Renderer& renderer, UI& ui)
{
	const ViewportPresentationSnapshot presentation = renderer.GetViewportPresentation();
	ui.SetViewportRenderProducts(presentation.Products);
	ui.SetViewportFinalColorTexture(presentation.Texture);
	ui.Update();
	renderer.SubmitUiRenderPacket(ui.ConsumeRenderPacket());
	renderer.OnRender();
}
