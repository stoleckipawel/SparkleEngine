#include "PCH.h"

#include "Cooking/BindingRecordBuilder.h"

#include "Cooking/StageMaskUtils.h"

#include <cstddef>

void BindingRecordBuilder::Build(
    const PassParameterLayout& layout,
	Engine::Strings::StringTableBuilder& stringTable,
    std::vector<CookedShaderBindingRecord>& outBindingRecords)
{
	const std::vector<PassParameterDesc>& parameters = layout.GetParameters();
	outBindingRecords.clear();
	outBindingRecords.reserve(parameters.size());

	for (std::size_t parameterIndex = 0; parameterIndex < parameters.size(); ++parameterIndex)
	{
		const PassParameterDesc& parameter = parameters[parameterIndex];
		outBindingRecords.push_back(
		    CookedShaderBindingRecord{
		        .Name = stringTable.Add(parameter.Name),
		        .SemanticKind = parameter.Kind,
		        .ResourceDomain = parameter.ResourceDomain,
		        .Access = parameter.Access,
		        .VisibilityMask = StageMaskUtils::FromVisibility(parameter.Visibility),
		        .LogicalBindingIndex = static_cast<std::uint32_t>(parameterIndex),
		        .ArrayCount = parameter.ArrayCount,
		        .ValueSizeInBytes = parameter.ValueSizeInBytes});
	}
}

}
