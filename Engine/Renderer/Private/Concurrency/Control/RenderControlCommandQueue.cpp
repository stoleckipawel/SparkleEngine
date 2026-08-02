#include "PCH.h"
#include "Concurrency/Control/RenderControlCommandQueue.h"

static const auto g_renderControlCommandQueueLogger = Logging::GetOrCreateLogger("Renderer.ControlQueue");

RenderControlCommandQueue::RenderControlCommandQueue(std::size_t capacity) :
    m_capacity(capacity)
{
	if (m_capacity == 0)
	{
		Diagnostics::Fatal(g_renderControlCommandQueueLogger, __FILE__, __LINE__, "Render-control queue capacity is zero.");
	}
}

void RenderControlCommandQueue::WaitPush(RenderControlCommand command)
{
	{
		std::unique_lock lock(m_mutex);
		m_notFull.wait(lock, [this] { return m_closed || m_commands.size() < m_capacity; });
		if (m_closed)
		{
			Diagnostics::Fatal(
			    g_renderControlCommandQueueLogger,
			    __FILE__,
			    __LINE__,
			    "Render-control queue closed while the producer was submitting a command.");
		}
		m_commands.push_back(std::move(command));
	}
	m_notEmpty.notify_one();
}

std::optional<RenderControlCommand> RenderControlCommandQueue::WaitPop()
{
	std::unique_lock lock(m_mutex);
	m_notEmpty.wait(lock, [this] { return m_closed || !m_commands.empty(); });
	if (m_commands.empty())
	{
		return std::nullopt;
	}
	RenderControlCommand command = std::move(m_commands.front());
	m_commands.pop_front();
	lock.unlock();
	m_notFull.notify_one();
	return command;
}

std::vector<RenderControlCommand> RenderControlCommandQueue::Drain()
{
	std::vector<RenderControlCommand> commands;
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

void RenderControlCommandQueue::Close() noexcept
{
	{
		std::lock_guard lock(m_mutex);
		m_closed = true;
	}
	m_notEmpty.notify_all();
	m_notFull.notify_all();
}
