#include "PCH.h"
#include "Concurrency/Control/RenderThreadCommandQueue.h"

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_renderThreadCommandQueueLogger, "Renderer.ThreadCommandQueue");

RenderThreadCommandQueue::RenderThreadCommandQueue(std::size_t capacity) :
    m_capacity(capacity)
{
	if (m_capacity == 0)
	{
		Diagnostics::Fatal(g_renderThreadCommandQueueLogger, __FILE__, __LINE__, "Render-thread command queue capacity is zero.");
	}
}

bool RenderThreadCommandQueue::WaitPush(RenderThreadCommand command)
{
	{
		std::unique_lock lock(m_mutex);
		m_notFull.wait(lock, [this] { return m_closed || m_commands.size() < m_capacity; });
		if (m_closed)
		{
			return false;
		}
		m_commands.push_back(std::move(command));
	}
	m_notEmpty.notify_one();
	return true;
}

std::optional<RenderThreadCommand> RenderThreadCommandQueue::WaitPop()
{
	std::unique_lock lock(m_mutex);
	m_notEmpty.wait(lock, [this] { return m_closed || !m_commands.empty(); });
	if (m_commands.empty())
	{
		return std::nullopt;
	}
	RenderThreadCommand command = std::move(m_commands.front());
	m_commands.pop_front();
	lock.unlock();
	m_notFull.notify_one();
	return command;
}

std::vector<RenderThreadCommand> RenderThreadCommandQueue::Drain()
{
	std::vector<RenderThreadCommand> commands;
	{
		std::lock_guard lock(m_mutex);
		commands.reserve(m_commands.size());
		while (!m_commands.empty())
		{
			commands.push_back(std::move(m_commands.front()));
			m_commands.pop_front();
		}
	}
	m_notFull.notify_all();
	return commands;
}

void RenderThreadCommandQueue::Close() noexcept
{
	{
		std::lock_guard lock(m_mutex);
		m_closed = true;
	}
	m_notEmpty.notify_all();
	m_notFull.notify_all();
}
