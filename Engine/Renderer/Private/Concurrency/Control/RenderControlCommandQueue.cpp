#include "PCH.h"
#include "Concurrency/Control/RenderControlCommandQueue.h"

#include <algorithm>

RenderControlCommandQueue::RenderControlCommandQueue(std::size_t capacity) : m_capacity((std::max)(capacity, std::size_t{1})) {}

bool RenderControlCommandQueue::Push(RenderControlCommand command)
{
	{
		std::lock_guard lock(m_mutex);
		if (m_closed || m_commands.size() == m_capacity) return false;
		m_commands.push_back(std::move(command));
	}
	m_notEmpty.notify_one();
	return true;
}

std::optional<RenderControlCommand> RenderControlCommandQueue::WaitPop()
{
	std::unique_lock lock(m_mutex);
	m_notEmpty.wait(lock, [this] { return m_closed || !m_commands.empty(); });
	if (m_commands.empty()) return std::nullopt;
	RenderControlCommand command = std::move(m_commands.front());
	m_commands.pop_front();
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
	return commands;
}

void RenderControlCommandQueue::Close() noexcept
{
	{
		std::lock_guard lock(m_mutex);
		m_closed = true;
	}
	m_notEmpty.notify_all();
}

bool RenderControlCommandQueue::IsClosed() const noexcept
{
	std::lock_guard lock(m_mutex);
	return m_closed;
}
