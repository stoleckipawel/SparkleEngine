#include "PCH.h"

#include "Editor/EditorUiFrameRenderer.h"

#include "Editor/Public/UI.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

struct EditorUiFrameRenderer::Context final
{
	RuntimeApplication& Runtime;
	Renderer& RendererFacade;
	UI& EditorUi;
};

void EditorUiFrameRenderer::Render(RuntimeApplication& runtime, Renderer& renderer, UI& ui)
{
	Context context{runtime, renderer, ui};
	renderer.RenderSerialUiFrame(&Compose, &context);
}

void EditorUiFrameRenderer::Compose(void* opaqueContext) noexcept
{
	Context& frame = *static_cast<Context*>(opaqueContext);
	frame.EditorUi.SetViewportRenderProducts(frame.Runtime.GetViewportRenderProducts());
	const ViewportPresentationProduct sceneColor =
	    frame.RendererFacade.BeginViewportPresentation(RenderOutputFlags::SceneColor);
	frame.EditorUi.SetViewportSceneColorTextureId(sceneColor.TextureId);
	frame.EditorUi.Update();

	constexpr float clearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	frame.RendererFacade.BeginHostPresentation(clearColor);
	frame.EditorUi.Render();
	frame.RendererFacade.EndHostPresentation();
	frame.RendererFacade.EndViewportPresentation(RenderOutputFlags::SceneColor);
}
