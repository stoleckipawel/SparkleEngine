#include "PCH.h"

#include "Editor/EditorUiFrameRenderer.h"

#include "Editor/Public/UI.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

void EditorUiFrameRenderer::Render(RuntimeApplication& runtime, Renderer& renderer, UI& ui)
{
	const ViewportRenderProducts products = runtime.GetViewportRenderProducts();
	ui.SetViewportRenderProducts(products);
	const RenderProduct* sceneColor = products.FindProduct(RenderOutputFlags::SceneColor);
	ui.SetViewportSceneColorTexture(
	    sceneColor != nullptr ? sceneColor->EditorTexture : EditorTextureHandle{});
	ui.Update();
	renderer.SubmitEditorRenderPacket(ui.ConsumeRenderPacket());
	renderer.OnRender();
}
