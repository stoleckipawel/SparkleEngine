#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanDiagnosticMessageQueue.h"

#include <utility>

VulkanDiagnosticMessageQueue::VulkanDiagnosticMessageQueue() noexcept = default;

VulkanDiagnosticMessageQueue::~VulkanDiagnosticMessageQueue() noexcept = default;

void VulkanDiagnosticMessageQueue::Push(RhiDiagnosticMessage message) noexcept
{
	std::lock_guard lock(m_mutex);
	if (m_messages.size() >= Capacity)
	{
		m_messages.pop_front();
	}

	m_messages.push_back(std::move(message));
}

bool VulkanDiagnosticMessageQueue::TryPop(RhiDiagnosticMessage& outMessage) noexcept
{
	std::lock_guard lock(m_mutex);
	if (m_messages.empty())
	{
		return false;
	}

	outMessage = std::move(m_messages.front());
	m_messages.pop_front();
	return true;
}

void VulkanDiagnosticMessageQueue::Clear() noexcept
{
	std::lock_guard lock(m_mutex);
	m_messages.clear();
}
