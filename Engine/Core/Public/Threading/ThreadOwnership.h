#pragma once

#include "Core/Public/CoreAPI.h"

#include <source_location>
#include <string>
#include <string_view>
#include <thread>

namespace Threading
{
	// Diagnostic metadata only; this does not grant ownership or synchronize memory.
	SPARKLE_CORE_API void SetCurrentThreadRole(std::string_view role) noexcept;

	class SPARKLE_CORE_API OwnerThread final
	{
	  public:
		explicit OwnerThread(std::string ownerDescription) noexcept;

		OwnerThread(const OwnerThread&) = delete;
		OwnerThread& operator=(const OwnerThread&) = delete;
		OwnerThread(OwnerThread&&) = delete;
		OwnerThread& operator=(OwnerThread&&) = delete;

		void AssertAccess(std::source_location location = std::source_location::current()) const noexcept;

	  private:
		std::thread::id m_thread = std::this_thread::get_id();
		std::string m_ownerDescription;
	};
}
