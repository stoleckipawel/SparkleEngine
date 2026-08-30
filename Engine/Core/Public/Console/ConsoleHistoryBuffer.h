#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class SPARKLE_CORE_API ConsoleHistoryBuffer final
{
public:
	explicit ConsoleHistoryBuffer(std::size_t maxEntries = kDefaultMaxEntries);

	void Add(std::string_view line);
	void Clear() noexcept;
	void ResetNavigation() noexcept;

	std::optional<std::string> NavigatePrevious(std::string_view currentLine);
	std::optional<std::string> NavigateNext();

	const std::vector<std::string>& GetEntries() const noexcept { return m_entries; }

private:
	static constexpr std::size_t kDefaultMaxEntries = 128;

	std::vector<std::string> m_entries;
	std::string m_pendingLine;
	std::size_t m_maxEntries = kDefaultMaxEntries;
	std::size_t m_navigationIndex = 0;
	bool m_isNavigating = false;
};
