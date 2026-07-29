#include "PCH.h"

#include "Core/Public/Threading/ThreadOwnership.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <format>
#include <utility>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

namespace Threading
{

		thread_local std::string g_currentThreadRole = "Sparkle.UnlabeledThread";
		auto g_threadOwnershipLogger = Logging::GetOrCreateLogger("Threading.Ownership");


	void SetCurrentThreadRole(std::string_view role) noexcept
	{
		try
		{
			g_currentThreadRole.assign(role);
#if defined(_WIN32)
			const std::wstring wideRole(role.begin(), role.end());
			(void) ::SetThreadDescription(::GetCurrentThread(), wideRole.c_str());
#endif
		}
		catch (...)
		{
			// A diagnostic label must never alter program control flow.
		}
	}

	OwnerThread::OwnerThread(std::string ownerDescription) noexcept : m_ownerDescription(std::move(ownerDescription)) {}

	void OwnerThread::AssertAccess(std::source_location location) const noexcept
	{
		if (std::this_thread::get_id() == m_thread)
		{
			return;
		}

		const std::string message = std::format(
		    "Thread-affinity violation: '{}' requires its creator thread {}, but '{}' (thread {}) accessed it from {}:{} ({}).",
		    m_ownerDescription,
		    std::hash<std::thread::id>{}(m_thread),
		    g_currentThreadRole,
		    std::hash<std::thread::id>{}(std::this_thread::get_id()),
		    location.file_name(),
		    location.line(),
		    location.function_name());
		Diagnostics::Fatal(g_threadOwnershipLogger, location.file_name(), location.line(), message);
	}
}
