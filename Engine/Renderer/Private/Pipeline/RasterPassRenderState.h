#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

class RasterPassRenderState final
{
public:
	void SetOpaqueBlend() noexcept;
	void SetDepthTest(CompareOp comparison) noexcept;
	void SetDepthWrite(bool enabled) noexcept;
	void DisableStencil() noexcept;

	const RhiBlendState& GetBlend() const noexcept { return m_blend; }
	const RhiDepthState& GetDepth() const noexcept { return m_depth; }
	const RhiStencilState& GetStencil() const noexcept { return m_stencil; }

private:
	RhiBlendState m_blend = {};
	RhiDepthState m_depth = {};
	RhiStencilState m_stencil = {};
};
