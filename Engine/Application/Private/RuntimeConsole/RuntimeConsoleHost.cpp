#include "PCH.h"

#include "RuntimeConsole/RuntimeConsoleHost.h"

#include "RuntimeConsole/RuntimeConsoleOverlay.h"

#include <utility>

RuntimeConsoleHost::RuntimeConsoleHost(Timer& timer, Window& window, CVarControlExecutor executor)
{
	m_overlay = std::make_unique<RuntimeConsoleOverlay>(timer, window, std::move(executor));
}

RuntimeConsoleHost::~RuntimeConsoleHost() noexcept = default;

std::optional<UiRenderPacket> RuntimeConsoleHost::Update()
{
	m_overlay->Update();
	if (m_overlay->IsVisible())
	{
		return m_overlay->ConsumeRenderPacket();
	}
	return std::nullopt;
}
