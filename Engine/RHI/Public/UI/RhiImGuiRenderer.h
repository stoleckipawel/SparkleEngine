#pragma once

#include "../RHIAPI.h"

struct ImDrawData;

class SPARKLE_RHI_API RhiImGuiRenderer
{
  public:
	virtual ~RhiImGuiRenderer() noexcept = default;

	virtual bool Initialize() = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual void RenderDrawData(ImDrawData* drawData) noexcept = 0;
	virtual void Shutdown() noexcept = 0;
};