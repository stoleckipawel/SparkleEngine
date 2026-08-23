#include "PCH.h"

#include "Frame/Retirement/FrameExecutionRetirementQueue.h"

#include "Frame/RenderFrame.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Device/RenderDeviceServices.h"

#include <algorithm>
#include <array>

FrameExecutionRetirementQueue::FrameExecutionRetirementQueue() noexcept = default;

FrameExecutionRetirementQueue::~FrameExecutionRetirementQueue() noexcept = default;

void FrameExecutionRetirementQueue::Retire(
    const RenderDeviceServices& deviceServices,
    std::unique_ptr<FrameGraph> graph,
    std::vector<std::unique_ptr<RenderFrame>> renderFrames) noexcept
{
	if (graph == nullptr)
	{
		return;
	}

	m_retiredExecutions.push_back(
	    RetiredFrameExecution{
	        .LastUse = CaptureLastSubmittedState(deviceServices),
	        .Graph = std::move(graph),
	        .RenderFrames = std::move(renderFrames)});
}

void FrameExecutionRetirementQueue::Poll(const RenderDeviceServices& deviceServices) noexcept
{
	m_retiredExecutions.erase(
	    std::remove_if(
	        m_retiredExecutions.begin(),
	        m_retiredExecutions.end(),
	        [&deviceServices](const RetiredFrameExecution& execution) noexcept
	        { return IsSubmissionStateComplete(deviceServices, execution.LastUse); }),
	    m_retiredExecutions.end());
}

RhiSubmissionState FrameExecutionRetirementQueue::CaptureLastSubmittedState(const RenderDeviceServices& deviceServices) noexcept
{
	RhiSubmissionState state;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		state.MarkUsed(deviceServices.GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
	return state;
}

bool FrameExecutionRetirementQueue::IsSubmissionStateComplete(
    const RenderDeviceServices& deviceServices,
    const RhiSubmissionState& state) noexcept
{
	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = state.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
	{
		if (!deviceServices.IsSubmissionComplete(tokens[tokenIndex]))
		{
			return false;
		}
	}
	return true;
}
