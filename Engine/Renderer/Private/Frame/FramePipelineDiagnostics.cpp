#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() noexcept
{
	return *m_frameExecutionDiagnostics[m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

const FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() const noexcept
{
	return *m_frameExecutionDiagnostics[m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex()];
}
