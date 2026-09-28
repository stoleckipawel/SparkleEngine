#pragma once

#include <cstddef>
#include <iosfwd>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

struct ToolWorkProgress final
{
	std::size_t completed = 0;
	std::size_t total = 0;
	std::string phase;

	bool IsDeterminate() const noexcept { return total != 0; }
};

class ToolWorkProgressProtocol final
{
public:
	ToolWorkProgressProtocol() = delete;

	static std::string Format(std::string_view phase, std::size_t completed = 0, std::size_t total = 0);
	static std::optional<ToolWorkProgress> Parse(std::string_view line);

private:
	static std::optional<std::size_t> ParseCount(std::string_view text);
};

class ToolWorkProgressWriter final
{
public:
	explicit ToolWorkProgressWriter(std::ostream& output) noexcept;
	void Report(std::string_view phase, std::size_t completed = 0, std::size_t total = 0);

private:
	std::ostream& m_output;
	std::mutex m_mutex;
	std::string m_lastPhase;
	std::size_t m_lastPercentage = std::numeric_limits<std::size_t>::max();
};
