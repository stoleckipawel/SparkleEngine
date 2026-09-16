#pragma once

#include "Concurrency/Control/RenderThreadCommand.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

class RenderThreadCommandQueue final
{
public:
	explicit RenderThreadCommandQueue(std::size_t capacity);

	void WaitPush(RenderThreadCommand command);
	std::optional<RenderThreadCommand> WaitPop();
	std::vector<RenderThreadCommand> Drain();
	void Close() noexcept;

private:
	const std::size_t m_capacity;
	std::mutex m_mutex;
	std::condition_variable m_notEmpty;
	std::condition_variable m_notFull;
	std::deque<RenderThreadCommand> m_commands;
	bool m_closed = false;
};
