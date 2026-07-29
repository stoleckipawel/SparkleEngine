#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Host/RendererHost.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() noexcept
{
	return *m_frameExecutionDiagnostics[m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

const FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() const noexcept
{
	return *m_frameExecutionDiagnostics[m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}
