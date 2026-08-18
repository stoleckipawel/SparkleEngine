#include "PCH.h"
#include "SceneData/Input/RenderInputConsumer.h"

#include "SceneData/RenderWorld.h"

RenderInputConsumer::RenderInputConsumer(RenderWorld& world) noexcept :
    m_world(&world)
{
}

bool RenderInputConsumer::Submit(RenderFrameSubmission submission)
{
	if (m_pending)
		return false;
	m_pending = std::move(submission);
	return true;
}

RenderInputConsumeResult RenderInputConsumer::ConsumePending() noexcept
{
	RenderInputConsumeResult result;
	if (!m_pending)
		return result;
	RenderFrameSubmission submission = std::move(*m_pending);
	m_pending.reset();

	if (submission.FrameId <= m_lastFrameId)
	{
		result.Diagnostic = "Render frame submission identity is stale.";
		return result;
	}
	if (m_world->ApplyFrame(submission.Scene.Structural, submission.Scene.Dynamic, result.Diagnostic) != RenderWorldApplyStatus::Applied)
	{
		return result;
	}

	m_lastFrameId = submission.FrameId;
	result.Accepted = true;
	result.SceneReset = submission.Scene.Structural.ResetScene;
	m_frameId = submission.FrameId;
	m_sceneDynamic = std::move(submission.Scene.Dynamic);
	m_viewInput = std::move(submission.View);
	return result;
}
