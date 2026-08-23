#include "PCH.h"
#include "Pipeline/RasterPassRenderState.h"

void RasterPassRenderState::SetOpaqueBlend() noexcept
{
	m_blend = {};
}

void RasterPassRenderState::SetDepthTest(CompareOp comparison) noexcept
{
	m_depth.DepthEnable = true;
	m_depth.DepthFunc = comparison;
}

void RasterPassRenderState::SetDepthWrite(bool enabled) noexcept
{
	m_depth.DepthWriteEnable = enabled;
}

void RasterPassRenderState::DisableStencil() noexcept
{
	m_stencil = {};
}
