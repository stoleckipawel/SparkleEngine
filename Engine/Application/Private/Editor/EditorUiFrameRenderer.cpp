#include "PCH.h"

#include "Editor/EditorUiFrameRenderer.h"

#include "Editor/Public/UI.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

void EditorUiFrameRenderer::Render(RuntimeApplication& runtime, Renderer& renderer, UI& ui)
{
	const ViewportRenderProducts products = runtime.GetViewportRenderProducts();
	ui.SetViewportRenderProducts(products);
	ui.SetViewportFinalColorTexture(renderer.GetViewportPresentationTexture());
	ui.Update();
	renderer.SubmitUiRenderPacket(ui.ConsumeRenderPacket());
	renderer.OnRender();
}
