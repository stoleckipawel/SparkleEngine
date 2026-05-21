#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <string_view>

enum class ToolConsoleSeverity : std::uint8_t
{
	Info,
	Warning,
	Error,
};

enum class ToolConsoleValueStyle : std::uint8_t
{
	Raw,
	Quoted,
};

struct ToolConsoleField final
{
	std::string name;
	std::string value;
	ToolConsoleValueStyle style = ToolConsoleValueStyle::Raw;
};

class ToolConsole final
{
public:
	static ToolConsoleField Field(std::string_view name, std::string_view value);
	static ToolConsoleField QuotedField(std::string_view name, std::string_view value);
	static ToolConsoleField PathField(std::string_view name, const std::filesystem::path& path);
	static std::string PathDisplayName(const std::filesystem::path& path);

	static void Info(std::string_view message);
	static void Warning(std::string_view message);
	static void Error(std::string_view message);
	static void Message(std::ostream& output, ToolConsoleSeverity severity, std::string_view message);
	static void Message(
	    std::ostream& output,
	    ToolConsoleSeverity severity,
	    std::string_view message,
	    std::initializer_list<ToolConsoleField> fields);

	static void Progress(
	    std::ostream& output,
	    std::string_view action,
	    std::string_view assetType,
	    std::size_t index,
	    std::size_t total,
	    std::string_view name,
	    std::initializer_list<ToolConsoleField> fields = {});

	static void Summary(
	    std::ostream& output,
	    std::string_view title,
	    std::initializer_list<ToolConsoleField> fields);

	static void ListHeader(std::ostream& output, std::string_view title);
	static void ListItem(std::ostream& output, std::size_t index, std::initializer_list<ToolConsoleField> fields);

private:
	static const char* GetSeverityPrefix(ToolConsoleSeverity severity) noexcept;
	static void WriteFields(std::ostream& output, std::initializer_list<ToolConsoleField> fields);
	static void WriteFieldValue(std::ostream& output, const ToolConsoleField& field);
};