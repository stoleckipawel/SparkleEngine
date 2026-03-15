#pragma once

#include "Events/ScopedEventHandle.h"

#include <functional>

class Window;

class RendererWindowObserver final
{
  public:
	using ResizeHandler = std::function<void()>;

	RendererWindowObserver(Window& window, ResizeHandler onResize) noexcept;
	~RendererWindowObserver() noexcept = default;

	RendererWindowObserver(const RendererWindowObserver&) = delete;
	RendererWindowObserver& operator=(const RendererWindowObserver&) = delete;
	RendererWindowObserver(RendererWindowObserver&&) = delete;
	RendererWindowObserver& operator=(RendererWindowObserver&&) = delete;

  private:
	void BindWindowResize() noexcept;

	Window* m_window = nullptr;
	ResizeHandler m_onResize;
	ScopedEventHandle m_resizeHandle;
};