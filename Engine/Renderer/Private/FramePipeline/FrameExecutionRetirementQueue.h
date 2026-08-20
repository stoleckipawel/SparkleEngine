#pragma once

#include "RHI/Public/Commands/RhiQueue.h"

#include <memory>
#include <vector>

struct RenderFrame;
class FrameGraph;
class RenderDeviceServices;

class FrameExecutionRetirementQueue final
{
public:
	FrameExecutionRetirementQueue() noexcept;
	~FrameExecutionRetirementQueue() noexcept;

	FrameExecutionRetirementQueue(const FrameExecutionRetirementQueue&) = delete;
	FrameExecutionRetirementQueue& operator=(const FrameExecutionRetirementQueue&) = delete;
	FrameExecutionRetirementQueue(FrameExecutionRetirementQueue&&) = delete;
	FrameExecutionRetirementQueue& operator=(FrameExecutionRetirementQueue&&) = delete;

	void Retire(
	    const RenderDeviceServices& deviceServices,
	    std::unique_ptr<FrameGraph> graph,
	    std::vector<std::unique_ptr<RenderFrame>> renderFrames) noexcept;
	void Poll(const RenderDeviceServices& deviceServices) noexcept;

private:
	struct RetiredFrameExecution final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<FrameGraph> Graph;
		std::vector<std::unique_ptr<RenderFrame>> RenderFrames;
	};

	static RhiSubmissionState CaptureLastSubmittedState(const RenderDeviceServices& deviceServices) noexcept;
	static bool IsSubmissionStateComplete(const RenderDeviceServices& deviceServices, const RhiSubmissionState& state) noexcept;

	std::vector<RetiredFrameExecution> m_retiredExecutions;
};
