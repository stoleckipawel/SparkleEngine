#pragma once

#include "UI/RhiImGuiRenderer.h"

struct ImDrawData;

class VulkanImGuiBackend final : public RhiImGuiRenderer
{
  public:
	bool Initialize() override;
	void BeginFrame() noexcept override;
	void RenderDrawData(ImDrawData* drawData) noexcept override;
	void Shutdown() noexcept override;
};