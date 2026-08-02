#pragma once

#include "Concurrency/Control/RenderControlCommand.h"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

class RenderControlCommandQueue final
{
public:
	explicit RenderControlCommandQueue(std::size_t capacity);

	void WaitPush(RenderControlCommand command);
	std::optional<RenderControlCommand> WaitPop();
	std::vector<RenderControlCommand> Drain();
	void Close() noexcept;

private:
	const std::size_t m_capacity;
	std::mutex m_mutex;
	std::condition_variable m_notEmpty;
	std::condition_variable m_notFull;
	std::deque<RenderControlCommand> m_commands;
	bool m_closed = false;
};
