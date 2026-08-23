#pragma once

#include "Core/Public/Strings/StringTableBuilder.h"
#include "RHI/Public/Shaders/ShaderReflection.h"
#include "ShaderReflection.h"

#include <span>
#include <vector>

// Flattens per-stage ShaderReflection into the global shader map POD arrays.
// Strings are interned through the caller-owned StringTableBuilder.
class ReflectionSerializer final
{
  public:
	struct Output
	{
		std::vector<CookedShaderReflectionRecord> reflectionRecords;
		std::vector<CookedShaderResourceBindingRecord> resourceBindings;
		std::vector<CookedShaderConstantBufferRecord> constantBuffers;
		std::vector<CookedShaderConstantBufferMemberRecord> constantBufferMembers;
		std::vector<CookedShaderInputElementRecord> inputElements;
		std::vector<CookedShaderPushConstantRangeRecord> pushConstantRanges;
		std::vector<CookedShaderSpecializationConstantRecord> specializationConstants;
	};

	static void Build(
	    std::span<const ShaderReflection> reflections,
	    Strings::StringTableBuilder& stringTable,
	    Output& outOutput);
};
