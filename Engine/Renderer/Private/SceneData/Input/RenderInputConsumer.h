#pragma once

#include "Rendering/RenderFrameSubmission.h"

#include <optional>
#include <string>
#include <utility>

class RenderWorld;

struct RenderInputConsumeResult final
{
	bool Accepted = false;
	bool SceneReset = false;
	std::string Diagnostic;
};

class RenderInputConsumer final
{
public:
	RenderInputConsumer(RenderWorld& world) noexcept;

	bool Submit(RenderFrameSubmission submission);
	RenderInputConsumeResult ConsumePending() noexcept;
	std::uint64_t GetFrameId() const noexcept { return m_frameId; }
	const RenderSceneDynamicData& GetSceneDynamicData() const noexcept { return m_sceneDynamic; }
	const RenderViewInput& GetViewInput() const noexcept { return m_viewInput; }

private:
	RenderWorld* m_world = nullptr;
	std::optional<RenderFrameSubmission> m_pending;
	RenderSceneDynamicData m_sceneDynamic;
	RenderViewInput m_viewInput;
	std::uint64_t m_frameId = 0;
	std::uint64_t m_lastFrameId = 0;
};
