#include "PCH.h"
#include "SceneData/Input/RenderInputConsumer.h"

#include "SceneData/RenderWorld.h"

RenderInputConsumer::RenderInputConsumer(
    RenderWorld& world) noexcept :
    m_world(&world)
{
}

bool RenderInputConsumer::Submit(RenderInputFrame input)
{
	if (m_pending) return false;
	m_pending = std::move(input);
	return true;
}

RenderInputConsumeResult RenderInputConsumer::ConsumePending() noexcept
{
	RenderInputConsumeResult result;
	if (!m_pending) return result;
	RenderInputFrame input = std::move(*m_pending);
	m_pending.reset();

	const RenderFrameMetadata& metadata = input.Dynamic.Metadata;
	if (metadata.SceneGeneration != input.WorldDelta.SceneGeneration ||
	    metadata.FrameId <= m_lastFrameId)
	{
		result.Diagnostic = "Render input metadata is stale or mismatched.";
		return result;
	}
	if (m_world->ApplyFrame(input.WorldDelta, input.Dynamic, result.Diagnostic) !=
	    RenderWorldApplyStatus::Applied)
	{
		return result;
	}

	input.Dynamic.Metadata.ResetHistory |=
	    m_lastFrameId != 0 &&
	    (metadata.SceneGeneration != m_sceneGeneration ||
	     metadata.ProviderGeneration != m_providerGeneration);
	m_lastFrameId = metadata.FrameId;
	m_sceneGeneration = metadata.SceneGeneration;
	m_providerGeneration = metadata.ProviderGeneration;
	result.Accepted = true;
	result.SceneReset = input.WorldDelta.ResetScene;
	m_dynamic = std::move(input.Dynamic);
	return result;
}
