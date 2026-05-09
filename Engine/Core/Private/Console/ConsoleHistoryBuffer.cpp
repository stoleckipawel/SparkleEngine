#include "PCH.h"

#include "Core/Public/Console/ConsoleHistoryBuffer.h"

#include "Core/Public/Strings/StringUtils.h"

ConsoleHistoryBuffer::ConsoleHistoryBuffer(std::size_t maxEntries) : m_maxEntries(maxEntries == 0 ? kDefaultMaxEntries : maxEntries) {}

void ConsoleHistoryBuffer::Add(std::string_view line)
{
	const std::string_view trimmedLine = Strings::TrimAsciiWhitespace(line);
	if (trimmedLine.empty())
	{
		ResetNavigation();
		return;
	}

	if (!m_entries.empty() && m_entries.back() == trimmedLine)
	{
		ResetNavigation();
		return;
	}

	m_entries.emplace_back(trimmedLine);
	while (m_entries.size() > m_maxEntries)
	{
		m_entries.erase(m_entries.begin());
	}
	ResetNavigation();
}

void ConsoleHistoryBuffer::Clear() noexcept
{
	m_entries.clear();
	ResetNavigation();
}

void ConsoleHistoryBuffer::ResetNavigation() noexcept
{
	m_pendingLine.clear();
	m_navigationIndex = m_entries.size();
	m_isNavigating = false;
}

std::optional<std::string> ConsoleHistoryBuffer::NavigatePrevious(std::string_view currentLine)
{
	if (m_entries.empty())
	{
		return std::nullopt;
	}

	if (!m_isNavigating)
	{
		m_pendingLine = std::string(currentLine);
		m_navigationIndex = m_entries.size();
		m_isNavigating = true;
	}

	if (m_navigationIndex == 0)
	{
		return m_entries.front();
	}

	--m_navigationIndex;
	return m_entries[m_navigationIndex];
}

std::optional<std::string> ConsoleHistoryBuffer::NavigateNext()
{
	if (!m_isNavigating)
	{
		return std::nullopt;
	}

	if (m_navigationIndex + 1 >= m_entries.size())
	{
		std::string pendingLine = std::move(m_pendingLine);
		ResetNavigation();
		return pendingLine;
	}

	++m_navigationIndex;
	return m_entries[m_navigationIndex];
}
