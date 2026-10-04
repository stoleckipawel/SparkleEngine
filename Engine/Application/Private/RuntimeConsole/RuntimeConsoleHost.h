#pragma once

#include "Core/Public/Console/CVarControl.h"
#include "Renderer/Public/UI/UiRenderPacket.h"

#include <memory>
#include <optional>

class RuntimeConsoleOverlay;
class Timer;
class Window;

class RuntimeConsoleHost final
{
public:
	RuntimeConsoleHost(Timer& timer, Window& window, CVarControlExecutor executor);
	~RuntimeConsoleHost() noexcept;

	RuntimeConsoleHost(const RuntimeConsoleHost&) = delete;
	RuntimeConsoleHost& operator=(const RuntimeConsoleHost&) = delete;
	RuntimeConsoleHost(RuntimeConsoleHost&&) = delete;
	RuntimeConsoleHost& operator=(RuntimeConsoleHost&&) = delete;

	std::optional<UiRenderPacket> Update();

private:
	std::unique_ptr<RuntimeConsoleOverlay> m_overlay;
};
