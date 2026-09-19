#include "PCH.h"

#include "Editor/Capture/ViewportCaptureSlot.h"

#include "Renderer.h"

#include <utility>

bool ViewportCaptureSlot::Request(Renderer& renderer, ViewportCaptureRequest request)
{
	if (!IsSettled())
	{
		return false;
	}
	m_capture = renderer.RequestViewportCapture(std::move(request));
	return static_cast<bool>(m_capture);
}

void ViewportCaptureSlot::Update(Renderer& renderer)
{
	if (!m_capture)
	{
		return;
	}
	ViewportCaptureReadback readback;
	if (!renderer.TryTakeViewportCapture(m_capture, readback))
	{
		return;
	}
	m_capture = {};
	if (m_discard)
	{
		m_discard = false;
		return;
	}
	m_readback = std::move(readback);
}

void ViewportCaptureSlot::Discard() noexcept
{
	m_readback.reset();
	m_discard = static_cast<bool>(m_capture);
}

ViewportCaptureReadback ViewportCaptureSlot::TakeReadback()
{
	ViewportCaptureReadback readback = std::move(*m_readback);
	m_readback.reset();
	return readback;
}
