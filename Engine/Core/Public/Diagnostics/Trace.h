#pragma once

#include "Core/Public/CoreAPI.h"

#include <string_view>

namespace Diagnostics
{
	class SPARKLE_CORE_API DiagnosticName final
	{
	  public:
		constexpr DiagnosticName() noexcept = default;
		constexpr explicit DiagnosticName(std::string_view canonicalName) noexcept : m_canonicalName(canonicalName) {}

		constexpr std::string_view GetCanonicalName() const noexcept { return m_canonicalName; }
		constexpr bool IsEmpty() const noexcept { return m_canonicalName.empty(); }
		constexpr explicit operator bool() const noexcept { return !IsEmpty(); }

	  private:
		std::string_view m_canonicalName{};
	};
}
