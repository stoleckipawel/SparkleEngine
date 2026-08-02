#include "Concurrency/Control/RenderControlCompletion.h"

#include "Core/Public/Diagnostics/Verify.h"

static const auto g_renderControlCompletionLogger = Logging::GetOrCreateLogger("Renderer.ControlCompletion");

void RenderControlCompletion::Complete(RenderControlResult result)
{
	{
		std::lock_guard lock(m_mutex);
		if (m_completed)
		{
			Diagnostics::Fatal(
			    g_renderControlCompletionLogger,
			    __FILE__,
			    __LINE__,
			    "Render-control completion was published more than once.");
		}

		m_result = std::move(result);
		m_completed = true;
	}

	m_completedCondition.notify_one();
}

void RenderControlCompletion::Cancel()
{
	Complete(RenderControlError{"RenderThread stopped before the render-control command completed."});
}

RenderControlResult RenderControlCompletion::Wait()
{
	std::unique_lock lock(m_mutex);
	m_completedCondition.wait(lock, [this] { return m_completed; });
	return std::move(m_result);
}
