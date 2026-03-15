#include "PCH.h"
#include "RendererWindowObserver.h"

#include "Window.h"

RendererWindowObserver::RendererWindowObserver(Window& window, ResizeHandler onResize) noexcept :
    m_window(&window), m_onResize(std::move(onResize))
{
	BindWindowResize();
}

void RendererWindowObserver::BindWindowResize() noexcept
{
	auto handle = m_window->OnResized.Add(
	    [this]()
	    {
		    m_onResize();
	    });
	m_resizeHandle = ScopedEventHandle(m_window->OnResized, handle);
}
