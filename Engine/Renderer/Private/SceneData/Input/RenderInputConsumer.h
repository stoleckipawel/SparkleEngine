#pragma once

#include "Rendering/RenderInputFrame.h"

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
	RenderInputConsumer(
	    RenderWorld& world) noexcept;

	bool Submit(RenderInputFrame input);
	RenderInputConsumeResult ConsumePending() noexcept;
	const RenderFrameDynamicData& GetDynamicData() const noexcept { return m_dynamic; }

  private:
	RenderWorld* m_world = nullptr;
	std::optional<RenderInputFrame> m_pending;
	RenderFrameDynamicData m_dynamic;
	std::uint64_t m_lastFrameId = 0;
	std::uint64_t m_frameGeneration = 0;
	std::uint64_t m_providerGeneration = 0;
};
