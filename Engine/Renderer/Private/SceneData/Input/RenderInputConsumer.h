#pragma once

#include "Rendering/RenderFrameSubmission.h"

#include <optional>
#include <string>
#include <utility>

class RenderScene;

struct RenderInputConsumeResult final
{
	bool Accepted = false;
	bool SceneReset = false;
	std::string Diagnostic;
};

class RenderInputConsumer final
{
public:
	RenderInputConsumer(RenderScene& scene) noexcept;

	bool Submit(RenderFrameSubmission submission);
	RenderInputConsumeResult ConsumePending() noexcept;
	std::uint64_t GetFrameId() const noexcept { return m_frameId; }
	const RenderViewInput& GetViewInput() const noexcept { return m_viewInput; }

private:
	RenderScene* m_scene = nullptr;
	std::optional<RenderFrameSubmission> m_pending;
	RenderViewInput m_viewInput;
	std::uint64_t m_frameId = 0;
	std::uint64_t m_lastFrameId = 0;
};
