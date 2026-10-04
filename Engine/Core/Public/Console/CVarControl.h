#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class ConsoleVariableRegistry;

enum class CVarControlOperation : std::uint8_t
{
	Query,
	Set,
	List
};

struct CVarControlEntry final
{
	std::string Name;
	std::string Value;
};

struct CVarControlRequest final
{
	CVarControlOperation Operation = CVarControlOperation::Query;
	std::vector<CVarControlEntry> Entries;
	std::string Filter;
};

struct CVarControlValue final
{
	std::string Name;
	std::string Value;
	std::string Type;
	std::string Description;
};

struct CVarControlResult final
{
	std::string Error;
	std::vector<CVarControlValue> Values;
};

// One host-bound operation, not a registry of feature callbacks. Inputs/results own their cross-thread strings.
using CVarControlExecutor = std::function<CVarControlResult(CVarControlRequest)>;

SPARKLE_CORE_API CVarControlResult ExecuteCVarControl(ConsoleVariableRegistry& registry, CVarControlRequest request);
