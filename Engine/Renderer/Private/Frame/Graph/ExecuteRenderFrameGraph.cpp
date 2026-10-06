#include "../../PCH.h"
#include "Frame/FramePipeline.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Device/RenderDeviceServices.h"

void FramePipeline::ExecuteFrame()
{
	if (!m_frameGraphExecutable)
	{
		return;
	}
	m_frameGraph->Setup();
	const FrameGraphPlan& plan = m_frameGraph->Compile();
	m_frameGraph->PreparePasses();
	m_frameGraph->Execute(plan, m_deviceServices, GetCurrentFrameDiagnostics(), m_taskExecutor);
}
