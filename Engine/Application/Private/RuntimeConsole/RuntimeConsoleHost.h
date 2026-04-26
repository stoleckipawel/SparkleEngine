#pragma once

#include <functional>
#include <memory>

class Renderer;
class RuntimeConsoleOverlay;
class Timer;
class Window;

class RuntimeConsoleHost final
{
  public:
	using RuntimeUpdate = std::function<void()>;

	RuntimeConsoleHost(Timer& timer, Window& window, Renderer& renderer);
	~RuntimeConsoleHost() noexcept;

	RuntimeConsoleHost(const RuntimeConsoleHost&) = delete;
	RuntimeConsoleHost& operator=(const RuntimeConsoleHost&) = delete;
	RuntimeConsoleHost(RuntimeConsoleHost&&) = delete;
	RuntimeConsoleHost& operator=(RuntimeConsoleHost&&) = delete;

	void TickFrame(Renderer& renderer, RuntimeUpdate updateRuntime);

  private:
	void RenderFrameWithOverlay(Renderer& renderer, RuntimeUpdate& updateRuntime);
	void RenderFrameWithoutOverlay(Renderer& renderer, RuntimeUpdate& updateRuntime);

	std::unique_ptr<RuntimeConsoleOverlay> m_overlay;
};
