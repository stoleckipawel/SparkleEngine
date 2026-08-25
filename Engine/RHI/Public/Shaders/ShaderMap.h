#pragma once

#include "../RHIAPI.h"
#include "../ShaderParameters/ShaderParameterSemantics.h"
#include "ShaderReflection.h"
#include "ShaderStage.h"
#include "ShaderTarget.h"

#include <cstdint>
#include <string_view>
#include <type_traits>

using ShaderTypeId = std::uint64_t;
using ShaderCodeHash = std::uint64_t;
using ShaderParameterSignature = std::uint64_t;

class PassParameterLayout;

SPARKLE_RHI_API ShaderTypeId BuildShaderTypeId(std::string_view shaderName) noexcept;
SPARKLE_RHI_API ShaderParameterSignature BuildShaderParameterSignature(const PassParameterLayout& layout) noexcept;

inline constexpr std::uint32_t kGlobalShaderMapMagic = 0x32414D53u;
inline constexpr std::uint32_t kCookedShaderLibraryMagic = 0x32494C53u;

enum class ShaderFeatureFlags : std::uint32_t
{
	None = 0,
	UsesInlineRayQuery = 1u << 0u,
	UsesAccelerationStructure = 1u << 1u,
	UsesDescriptorIndexing = 1u << 2u,
};

struct RayTracingShaderMetadata final
{
	std::uint32_t PayloadSizeInBytes = 0;
	std::uint32_t AttributeSizeInBytes = 0;
	std::uint32_t MinimumRecursionDepth = 0;
	std::uint32_t LocalRecordSizeInBytes = 0;
	ShaderParameterSignature LocalRecordSignature = 0;

	constexpr bool operator==(const RayTracingShaderMetadata&) const noexcept = default;
};

SPARKLE_RHI_API ShaderFeatureFlags operator|(ShaderFeatureFlags lhs, ShaderFeatureFlags rhs) noexcept;
SPARKLE_RHI_API ShaderFeatureFlags& operator|=(ShaderFeatureFlags& lhs, ShaderFeatureFlags rhs) noexcept;
SPARKLE_RHI_API bool HasShaderFeature(ShaderFeatureFlags value, ShaderFeatureFlags flag) noexcept;

struct ShaderMapStringRef
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;

	constexpr bool IsValid() const noexcept { return SizeInBytes > 0; }
};

struct ShaderCodeBlobRef
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;

	constexpr bool IsValid() const noexcept { return SizeInBytes > 0; }
};

struct GlobalShaderMapHeader
{
	std::uint32_t Magic = kGlobalShaderMapMagic;
	std::uint32_t EntryCount = 0;
	std::uint64_t PublicationHash = 0;
	std::uint32_t BindingRecordCount = 0;
	std::uint32_t ReflectionRecordCount = 0;
	std::uint32_t ResourceBindingRecordCount = 0;
	std::uint32_t ConstantBufferRecordCount = 0;
	std::uint32_t ConstantBufferMemberRecordCount = 0;
	std::uint32_t InputElementRecordCount = 0;
	std::uint32_t PushConstantRangeRecordCount = 0;
	std::uint32_t SpecializationConstantRecordCount = 0;
	std::uint32_t StringTableSizeInBytes = 0;
};

struct GlobalShaderMapEntry
{
	ShaderTypeId ShaderType = 0;
	ShaderTarget Target = kDefaultShaderTarget;
	ShaderStage Stage = ShaderStage::Count;
	ShaderBinaryFormat BinaryFormat = ShaderBinaryFormat::Dxil;
	ShaderFeatureFlags Features = ShaderFeatureFlags::None;
	ShaderCodeHash CodeHash = 0;
	ShaderParameterSignature ParameterSignature = 0;
	std::uint64_t CompileInputHash = 0;
	std::uint64_t BackendVersion = 0;
	ShaderMapStringRef ShaderName = {};
	ShaderMapStringRef EntryPoint = {};
	ShaderMapStringRef BackendName = {};
	ShaderMapStringRef CodegenTarget = {};
	std::uint32_t BindingRecordOffset = 0;
	std::uint32_t BindingRecordCount = 0;
	std::uint32_t ReflectionRecordIndex = 0;
	std::uint32_t RayPayloadSizeInBytes = 0;
	std::uint32_t RayAttributeSizeInBytes = 0;
	std::uint32_t MinimumRayRecursionDepth = 0;
	std::uint32_t LocalRecordSizeInBytes = 0;
	ShaderParameterSignature LocalRecordSignature = 0;
};

struct ShaderMapBindingRecord
{
	ShaderMapStringRef Name = {};
	ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	std::uint8_t Reserved0 = 0;
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
	std::uint16_t Reserved1 = 0;
	std::uint32_t LogicalBindingIndex = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;
};

struct CookedShaderLibraryHeader
{
	std::uint32_t Magic = kCookedShaderLibraryMagic;
	std::uint32_t RecordCount = 0;
	std::uint64_t PublicationHash = 0;
	std::uint32_t CodeBlobSizeInBytes = 0;
	std::uint32_t Reserved = 0;
};

struct CookedShaderCodeRecord
{
	ShaderCodeHash CodeHash = 0;
	ShaderCodeBlobRef Code = {};
};

static_assert(std::is_trivially_copyable_v<GlobalShaderMapHeader>);
static_assert(std::is_trivially_copyable_v<GlobalShaderMapEntry>);
static_assert(std::is_trivially_copyable_v<ShaderMapBindingRecord>);
static_assert(sizeof(ShaderMapBindingRecord) == 28, "serialized binding records must have no hidden padding.");
static_assert(std::is_trivially_copyable_v<CookedShaderLibraryHeader>);
static_assert(std::is_trivially_copyable_v<CookedShaderCodeRecord>);
