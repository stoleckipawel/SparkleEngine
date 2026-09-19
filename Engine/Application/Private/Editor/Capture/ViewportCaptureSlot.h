#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <optional>

class Renderer;

class ViewportCaptureSlot final
{
public:
	bool Request(Renderer& renderer, ViewportCaptureRequest request);
	void Update(Renderer& renderer);
	void Discard() noexcept;

	bool IsSettled() const noexcept { return !m_capture && !m_readback; }
	bool HasReadback() const noexcept { return m_readback.has_value(); }
	ViewportCaptureReadback TakeReadback();

private:
	ViewportCaptureId m_capture;
	std::optional<ViewportCaptureReadback> m_readback;
	bool m_discard = false;
};
