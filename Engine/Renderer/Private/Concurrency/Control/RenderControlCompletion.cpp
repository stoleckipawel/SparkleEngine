#include "Concurrency/Control/RenderControlCompletion.h"

void RenderControlCompletion::Complete(RenderControlResult result)
{
	{
		std::lock_guard lock(m_mutex);
		if (m_completed)
		{
			return;
		}

		m_result = std::move(result);
		m_completed = true;
	}

	m_completedCondition.notify_all();
}

void RenderControlCompletion::Cancel()
{
	Complete(std::monostate {});
}

RenderControlResult RenderControlCompletion::Wait()
{
	std::unique_lock lock(m_mutex);
	m_completedCondition.wait(lock, [this] { return m_completed; });
	return std::move(m_result);
}
