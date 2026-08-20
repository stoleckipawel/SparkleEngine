#include "PCH.h"
#include "SceneData/Input/RenderInputConsumer.h"

#include "Scene/RenderScene.h"

RenderInputConsumer::RenderInputConsumer(RenderScene& scene) noexcept :
    m_scene(&scene)
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
	if (m_scene->Apply(submission.Scene.Structural, std::move(submission.Scene.Dynamic), result.Diagnostic)
	    != RenderSceneApplyStatus::Applied)
	{
		return result;
	}

	m_lastFrameId = submission.FrameId;
	result.Accepted = true;
	result.SceneReset = submission.Scene.Structural.ResetScene;
	m_frameId = submission.FrameId;
	m_viewInput = std::move(submission.View);
	return result;
}
