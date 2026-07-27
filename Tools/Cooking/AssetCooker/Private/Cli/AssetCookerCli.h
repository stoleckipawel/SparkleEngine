#pragma once

#include "../Api/AssetCookerTypes.h"

#include <iosfwd>
#include <string_view>

struct AssetCookerServiceResult;

class AssetCookerCli final
{
  public:
	int Run(int argc, char** argv) const;

  private:
	struct Arguments;

	static bool IsHelp(std::string_view argument) noexcept;
	static bool IsConfiguration(std::string_view argument) noexcept;
	static bool ParseCommonArguments(int argc, char** argv, int startIndex, Arguments& arguments);
	static bool Parse(int argc, char** argv, Arguments& arguments);
	static void PrintUsage(std::ostream& output);
	static void PrintResult(const AssetCookerServiceResult& result);
};
