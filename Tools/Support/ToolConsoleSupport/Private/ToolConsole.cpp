#include "ToolConsole.h"

#include <iostream>
#include <ostream>

ToolConsoleField ToolConsole::Field(std::string_view name, std::string_view value)
{
	ToolConsoleField field;
	field.name = std::string(name);
	field.value = std::string(value);
	field.style = ToolConsoleValueStyle::Raw;
	return field;
}

ToolConsoleField ToolConsole::QuotedField(std::string_view name, std::string_view value)
{
	ToolConsoleField field;
	field.name = std::string(name);
	field.value = std::string(value);
	field.style = ToolConsoleValueStyle::Quoted;
	return field;
}

ToolConsoleField ToolConsole::PathField(std::string_view name, const std::filesystem::path& path)
{
	return QuotedField(name, path.string());
}

std::string ToolConsole::PathDisplayName(const std::filesystem::path& path)
{
	const std::filesystem::path fileName = path.filename();
	return fileName.empty() ? path.string() : fileName.string();
}

void ToolConsole::Info(std::string_view message)
{
	Message(std::cout, ToolConsoleSeverity::Info, message);
}

void ToolConsole::Warning(std::string_view message)
{
	Message(std::cout, ToolConsoleSeverity::Warning, message);
}

void ToolConsole::Error(std::string_view message)
{
	Message(std::cerr, ToolConsoleSeverity::Error, message);
}

void ToolConsole::Message(std::ostream& output, ToolConsoleSeverity severity, std::string_view message)
{
	output << GetSeverityPrefix(severity) << ' ' << message << '\n';
}

void ToolConsole::Message(
    std::ostream& output,
    ToolConsoleSeverity severity,
    std::string_view message,
    std::initializer_list<ToolConsoleField> fields)
{
	output << GetSeverityPrefix(severity) << ' ' << message;
	WriteFields(output, fields);
	output << '\n';
}

void ToolConsole::Progress(
    std::ostream& output,
    std::string_view action,
    std::string_view assetType,
    std::size_t index,
    std::size_t total,
    std::string_view name,
    std::initializer_list<ToolConsoleField> fields)
{
	output << GetSeverityPrefix(ToolConsoleSeverity::Info) << ' ' << action << ' ' << assetType << " [" << index << '/'
	       << total << "] name='" << name << "'";
	WriteFields(output, fields);
	output << '\n';
}

void ToolConsole::Summary(
    std::ostream& output,
    std::string_view title,
    std::initializer_list<ToolConsoleField> fields)
{
	output << GetSeverityPrefix(ToolConsoleSeverity::Info) << ' ' << title << ":\n";
	for (const ToolConsoleField& field : fields)
	{
		output << "  " << field.name << '=';
		WriteFieldValue(output, field);
		output << '\n';
	}
}

void ToolConsole::ListHeader(std::ostream& output, std::string_view title)
{
	output << GetSeverityPrefix(ToolConsoleSeverity::Info) << ' ' << title << ":\n";
}

void ToolConsole::ListItem(std::ostream& output, std::size_t index, std::initializer_list<ToolConsoleField> fields)
{
	output << "  " << index << ')';
	WriteFields(output, fields);
	output << '\n';
}

const char* ToolConsole::GetSeverityPrefix(ToolConsoleSeverity severity) noexcept
{
	switch (severity)
	{
	case ToolConsoleSeverity::Info:
		return "[LOG]";
	case ToolConsoleSeverity::Warning:
		return "[WARN]";
	case ToolConsoleSeverity::Error:
		return "[ERROR]";
	default:
		return "[LOG]";
	}
}

void ToolConsole::WriteFields(std::ostream& output, std::initializer_list<ToolConsoleField> fields)
{
	for (const ToolConsoleField& field : fields)
	{
		output << ' ' << field.name << '=';
		WriteFieldValue(output, field);
	}
}

void ToolConsole::WriteFieldValue(std::ostream& output, const ToolConsoleField& field)
{
	if (field.style == ToolConsoleValueStyle::Quoted)
	{
		output << '\'' << field.value << '\'';
		return;
	}

	output << field.value;
}