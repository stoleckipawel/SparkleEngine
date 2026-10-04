#include "PCH.h"

#include "Core/Public/Console/CVarControl.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Strings/StringUtils.h"

#include <utility>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(LogCoreCVarControl, "Core.Console.Control");

CVarControlResult ExecuteCVarControl(ConsoleVariableRegistry& registry, CVarControlRequest request)
{
	constexpr std::size_t maximumBatchSize = 64;
	CVarControlResult result;
	std::vector<ConsoleVariableBase*> variables;
	if (request.Operation == CVarControlOperation::List)
	{
		if (!request.Entries.empty())
			return {.Error = "List does not accept entries."};
		for (ConsoleVariableBase* variable : registry.GetVariables())
			if (variable != nullptr
			    && (request.Filter.empty() || Strings::ContainsIgnoreCase(variable->GetName(), request.Filter)
			        || Strings::ContainsIgnoreCase(variable->GetDescription(), request.Filter)))
				variables.push_back(variable);
	}
	else
	{
		if (request.Operation != CVarControlOperation::Query && request.Operation != CVarControlOperation::Set)
			return {.Error = "Invalid CVar operation."};
		if (request.Entries.empty() || request.Entries.size() > maximumBatchSize || !request.Filter.empty())
			return {.Error = "Query/Set requires 1 through 64 entries and no filter."};
		variables.reserve(request.Entries.size());
		for (const CVarControlEntry& entry : request.Entries)
		{
			ConsoleVariableBase* variable = registry.Find(entry.Name);
			if (variable == nullptr)
				return {.Error = "unknown CVar: " + entry.Name};
			for (const ConsoleVariableBase* previous : variables)
				if (previous == variable)
					return {.Error = "duplicate CVar: " + entry.Name};
			if (request.Operation == CVarControlOperation::Set)
			{
				std::string error;
				if (!variable->ValidateValueFromString(entry.Value, error))
					return {.Error = "failed to set " + entry.Name + ": " + error};
			}
			else if (!entry.Value.empty())
				return {.Error = "Query does not accept values."};
			variables.push_back(variable);
		}
	}
	// Validation and assignment use the same pure parser and immutable text. No parsed-value cache is retained.
	// The host executes this entire operation between frames.
	if (request.Operation == CVarControlOperation::Set)
		for (std::size_t index = 0; index < variables.size(); ++index)
		{
			std::string error;
			if (!variables[index]->TrySetValueFromString(request.Entries[index].Value, error))
				Diagnostics::Fatal(
				    LogCoreCVarControl,
				    __FILE__,
				    __LINE__,
				    "Previously validated CVar input failed during commit: " + error);
		}
	result.Values.reserve(variables.size());
	for (const ConsoleVariableBase* variable : variables)
		result.Values.push_back(
		    {std::string(variable->GetName()),
		        variable->GetValueAsString(),
		        variable->GetValueTypeName(),
		        std::string(variable->GetDescription())});
	return result;
}
