#include "PCH.h"

#include "RuntimeConsole/RuntimeConsoleHost.h"

#include "RuntimeConsole/RuntimeConsoleOverlay.h"
#include "Renderer.h"

RuntimeConsoleHost::RuntimeConsoleHost(Timer& timer, Window& window, Renderer& renderer)
{
	m_overlay = std::make_unique<RuntimeConsoleOverlay>(timer, window, renderer.GetRenderHardwareInterface());
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

	renderer.PrepareHostFrame();
	renderer.RecordHostFrame();
	m_overlay->Update();

	RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
	renderHardware.BeginPresentOverlayPass();
	m_overlay->Render();
	renderHardware.EndPresentRenderPass();

	renderer.SubmitHostFrame();
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
