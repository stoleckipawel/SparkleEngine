#pragma once

#include "ShaderReflection.h"

#include <slang.h>

#include <cstdint>
#include <string>

class SlangReflectionExtractor final
{
public:
	SlangReflectionExtractor() = delete;

	static ShaderReflection Extract(slang::ProgramLayout& programLayout, ShaderStage stage);

private:
	static void VisitScope(slang::VariableLayoutReflection* scopeLayout, ShaderStage stage, ShaderReflection& outReflection);
	static void VisitVariable(slang::VariableLayoutReflection* variableLayout, ShaderStage stage, ShaderReflection& outReflection);
	static void VisitTypeFields(slang::TypeLayoutReflection* typeLayout, ShaderStage stage, ShaderReflection& outReflection);

	static void AddResourceBinding(
	    slang::VariableLayoutReflection& variableLayout,
	    slang::ParameterCategory category,
	    ShaderReflection& outReflection);
	static void AddConstantBuffer(
	    slang::VariableLayoutReflection& variableLayout,
	    ShaderReflectionResourceBinding& binding,
	    ShaderReflection& outReflection);
	static void AddPushConstantBlock(slang::VariableLayoutReflection& variableLayout, ShaderStage stage, ShaderReflection& outReflection);
	static void AddVaryingInput(slang::VariableLayoutReflection& variableLayout, ShaderReflection& outReflection);

	static void FlattenMembers(
	    slang::TypeLayoutReflection* typeLayout,
	    std::uint32_t parentOffset,
	    std::vector<ShaderReflectionConstantBufferMember>& outMembers);
	static CookedShaderResourceKind MapResourceKind(slang::TypeLayoutReflection* typeLayout, slang::ParameterCategory category);
	static CookedShaderResourceDimension MapResourceDimension(slang::TypeLayoutReflection* typeLayout);
	static CookedShaderScalarType MapScalarType(slang::TypeReflection::ScalarType type);
	static slang::TypeLayoutReflection* UnwrapSingleElementContainer(slang::TypeLayoutReflection* typeLayout);
	static std::uint32_t NormalizeArrayCount(std::size_t elementCount);
};
