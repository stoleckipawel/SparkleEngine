#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

struct RenderView;

class ReferencePathTracer final
{
public:
	ViewportRenderProgress Update(const RenderView& view) const noexcept;
};
