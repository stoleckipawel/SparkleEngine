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

	bool Push(RenderControlCommand command);
	std::optional<RenderControlCommand> WaitPop();
	std::vector<RenderControlCommand> Drain();
	void Close() noexcept;
	bool IsClosed() const noexcept;
	std::size_t GetCapacity() const noexcept { return m_capacity; }

  private:
	const std::size_t m_capacity;
	mutable std::mutex m_mutex;
	std::condition_variable m_notEmpty;
	std::deque<RenderControlCommand> m_commands;
	bool m_closed = false;
};
