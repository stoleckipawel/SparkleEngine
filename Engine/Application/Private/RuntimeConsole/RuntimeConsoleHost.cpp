#include "PCH.h"

#include "RuntimeConsole/RuntimeConsoleHost.h"

#include "RuntimeConsole/RuntimeConsoleOverlay.h"
#include "Renderer.h"

RuntimeConsoleHost::RuntimeConsoleHost(Timer& timer, Window& window)
{
	m_overlay = std::make_unique<RuntimeConsoleOverlay>(timer, window);
}

RuntimeConsoleHost::~RuntimeConsoleHost() noexcept = default;

void RuntimeConsoleHost::TickFrame(Renderer& renderer, RuntimeUpdate updateRuntime)
{
	if (updateRuntime)
	{
		updateRuntime();
	}

	if (m_overlay != nullptr)
	{
		m_overlay->Update();
		if (m_overlay->IsVisible())
		{
			renderer.SubmitUiRenderPacket(m_overlay->ConsumeRenderPacket());
		}
	}

	renderer.OnRender();
}
