#pragma once

#include "RHI/Public/Shaders/ShaderReflection.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// In-memory ShaderReflection used by the cook pipeline.
// Strings stay inline here and are interned during serialization.

struct ShaderReflectionConstantBufferMember final
{
	std::string Name;
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ArrayStrideInBytes = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
	std::uint8_t RowCount = 0;
	std::uint8_t ColumnCount = 0;
};

struct ShaderReflectionConstantBuffer final
{
	std::string Name;
	std::uint32_t SizeInBytes = 0;
	std::vector<ShaderReflectionConstantBufferMember> Members;
};

struct ShaderReflectionResourceBinding final
{
	std::string Name;
	CookedShaderResourceKind Kind = CookedShaderResourceKind::Unknown;
	CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
	bool IsReadOnly = true;
	std::uint32_t Set = 0;
	std::uint32_t Slot = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t SizeInBytes = 0;
	// Index into ShaderReflection::ConstantBuffers; valid only when this
	// binding is a constant buffer or push-constant block.
	std::uint32_t ConstantBufferIndex = kCookedShaderReflectionInvalidIndex;
};

struct ShaderReflectionInputElement final
{
	std::string Semantic;
	std::uint32_t SemanticIndex = 0;
	std::uint32_t Location = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
	std::uint8_t ComponentCount = 0;
};

struct ShaderReflectionPushConstantRange final
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
};

struct ShaderReflectionSpecializationConstant final
{
	std::string Name;
	std::uint32_t ConstantId = 0;
	std::uint64_t DefaultValueBits = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
};

struct ShaderReflection final
{
	std::vector<ShaderReflectionResourceBinding> Bindings;
	std::vector<ShaderReflectionConstantBuffer> ConstantBuffers;
	std::vector<ShaderReflectionInputElement> InputElements;
	std::vector<ShaderReflectionPushConstantRange> PushConstants;
	std::vector<ShaderReflectionSpecializationConstant> SpecializationConstants;
	std::array<std::uint32_t, 3> ThreadGroupSize = {0, 0, 0};
	std::uint32_t EntryFlags = 0;
	std::uint16_t WaveSize = 0;
};
