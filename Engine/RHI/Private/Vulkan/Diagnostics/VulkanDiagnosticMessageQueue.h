#pragma once

#include "Diagnostics/RhiDiagnostics.h"

#include <cstddef>
#include <deque>
#include <mutex>

class VulkanDiagnosticMessageQueue final
{
  public:
	VulkanDiagnosticMessageQueue() noexcept;
	~VulkanDiagnosticMessageQueue() noexcept;

	VulkanDiagnosticMessageQueue(const VulkanDiagnosticMessageQueue&) = delete;
	VulkanDiagnosticMessageQueue& operator=(const VulkanDiagnosticMessageQueue&) = delete;
	VulkanDiagnosticMessageQueue(VulkanDiagnosticMessageQueue&&) = delete;
	VulkanDiagnosticMessageQueue& operator=(VulkanDiagnosticMessageQueue&&) = delete;

	void Push(RhiDiagnosticMessage message) noexcept;
	bool TryPop(RhiDiagnosticMessage& outMessage) noexcept;
	void Clear() noexcept;

  private:
	static constexpr std::size_t Capacity = 256;

	std::deque<RhiDiagnosticMessage> m_messages;
	std::mutex m_mutex;
};
