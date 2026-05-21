#pragma once

#include "SparkleCliArguments.h"

#include <iosfwd>
#include <string_view>

namespace SparkleLauncher
{
	class SparkleCliParser final
	{
	public:
		bool Parse(int argc, char** argv, SparkleCliArguments& outArguments, std::ostream& error) const;

	private:
		bool TryHandleFlagOption(std::string_view argument, SparkleCliArguments& outArguments) const;
		bool TryHandleValueOption(
		    int argc,
		    char** argv,
		    int& index,
		    std::string_view argument,
		    SparkleCliArguments& outArguments,
		    std::ostream& error,
		    bool& outHandled) const;
		bool TryAssignStringValue(std::string_view argument, const char* value, SparkleCliArguments& outArguments, bool& outHandled) const;
		bool TryAssignModeValue(std::string_view argument, const char* value, SparkleCliArguments& outArguments, std::ostream& error, bool& outHandled) const;
		bool AssignOperationId(std::string_view argument, SparkleCliArguments& outArguments, std::ostream& error) const;
		bool IsValueOption(std::string_view argument) const;
		bool TryParseFormatMode(std::string_view text, FormatMode& outMode) const;
		bool TryParseCleanScope(std::string_view text, CleanScope& outScope) const;
		const char* RequireValue(int argc, char** argv, int& index, std::string_view optionName, std::ostream& error) const;
	};
}