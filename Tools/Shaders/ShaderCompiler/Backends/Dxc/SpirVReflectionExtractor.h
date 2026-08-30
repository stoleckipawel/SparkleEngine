#pragma once

#include "ShaderReflection.h"

#include <cstdint>
#include <span>
#include <string>

#include <spirv_reflect.h>

// SPIR-V -> ShaderReflection extractor backed by SPIRV-Reflect.
// SPIRV-Reflect symbols stay confined to Backends/Dxc/.
class SpirVReflectionExtractor final
{
public:
	SpirVReflectionExtractor() = delete;

	static ShaderReflection Extract(std::span<const std::uint8_t> bytecode, ShaderStage stage);

private:
	static CookedShaderResourceKind MapDescriptorType(SpvReflectDescriptorType type, SpvDim dim, SpvReflectDecorationFlags decorationFlags);
	static CookedShaderResourceDimension MapImageDim(SpvDim dim, std::uint32_t arrayed, std::uint32_t ms);
	static CookedShaderScalarType MapInputFormat(SpvReflectFormat format, std::uint8_t& outComponentCount);
	static CookedShaderScalarType MapNumericScalar(const SpvReflectNumericTraits& traits, bool isSigned);
	static void FlattenBlockMembers(
	    const SpvReflectBlockVariable& block,
	    std::uint32_t parentAbsoluteOffset,
	    std::vector<ShaderReflectionConstantBufferMember>& outMembers);
};
