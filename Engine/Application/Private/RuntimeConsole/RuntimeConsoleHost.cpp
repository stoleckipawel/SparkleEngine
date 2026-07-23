#include "PCH.h"

#include "RuntimeConsole/RuntimeConsoleHost.h"

#include "RuntimeConsole/RuntimeConsoleOverlay.h"
#include "Renderer.h"

class RuntimeConsoleOverlayFrameRenderer final
{
  public:
	static void Render(RuntimeConsoleOverlay& overlay, Renderer& renderer);

  private:
	struct Context final
	{
		RuntimeConsoleOverlay& Overlay;
		Renderer& RendererFacade;
	};

	static void Compose(void* opaqueContext) noexcept;
};

void RuntimeConsoleOverlayFrameRenderer::Render(RuntimeConsoleOverlay& overlay, Renderer& renderer)
{
	Context context {overlay, renderer};
	renderer.RenderSerialUiFrame(&Compose, &context);
}

void RuntimeConsoleOverlayFrameRenderer::Compose(void* opaqueContext) noexcept
{
	Context& frame = *static_cast<Context*>(opaqueContext);
	frame.Overlay.Update();
	frame.RendererFacade.BeginHostOverlayPresentation();
	frame.Overlay.Render();
	frame.RendererFacade.EndHostPresentation();
}

RuntimeConsoleHost::RuntimeConsoleHost(Timer& timer, Window& window, Renderer& renderer)
{
	m_overlay = std::make_unique<RuntimeConsoleOverlay>(timer, window, renderer.GetImGuiRenderer());
}

RuntimeConsoleHost::~RuntimeConsoleHost() noexcept = default;

void RuntimeConsoleHost::TickFrame(Renderer& renderer, RuntimeUpdate updateRuntime)
{
	if (m_overlay != nullptr && m_overlay->IsVisible())
	{
		RenderFrameWithOverlay(renderer, updateRuntime);
		return;
	}

	RenderFrameWithoutOverlay(renderer, updateRuntime);
}

void RuntimeConsoleHost::RenderFrameWithOverlay(Renderer& renderer, RuntimeUpdate& updateRuntime)
{
	if (updateRuntime)
	{
		updateRuntime();
	}

	RuntimeConsoleOverlayFrameRenderer::Render(*m_overlay, renderer);
}

void RuntimeConsoleHost::RenderFrameWithoutOverlay(Renderer& renderer, RuntimeUpdate& updateRuntime)
{
	if (updateRuntime)
	{
		updateRuntime();
	}

	renderer.OnRender();
	if (m_overlay != nullptr)
	{
		m_overlay->Update();
	}
}
