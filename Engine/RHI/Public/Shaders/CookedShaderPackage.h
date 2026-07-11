#pragma once

#include "ShaderReflection.h"
#include "ShaderStage.h"

#include "../ShaderParameters/ShaderParameterSemantics.h"

#include <cstdint>
#include <type_traits>

constexpr std::uint32_t MakeCookedShaderPackageMagic(char a, char b, char c, char d) noexcept
{
	return static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 8u) |
	       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 16u) |
	       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(d)) << 24u);
}

constexpr std::uint32_t kCookedShaderPackageMagic = MakeCookedShaderPackageMagic('S', 'S', 'H', 'D');
constexpr std::uint32_t kCookedShaderPackageVersion = 2;

enum class CookedShaderPackageKind : std::uint8_t
{
	Graphics = 0,
	Compute = 1,
	RayTracingLibrary = 2,
};

enum class CookedShaderPackageFeatureFlags : std::uint32_t
{
	None = 0,
	UsesInlineRayQuery = 1u << 0u,
	UsesAccelerationStructure = 1u << 1u,
	UsesAccelerationStructureDeviceAddress = 1u << 2u,
	UsesDescriptorIndexing = 1u << 3u,
};

constexpr CookedShaderPackageFeatureFlags operator|(CookedShaderPackageFeatureFlags lhs, CookedShaderPackageFeatureFlags rhs) noexcept
{
	return static_cast<CookedShaderPackageFeatureFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr CookedShaderPackageFeatureFlags& operator|=(CookedShaderPackageFeatureFlags& lhs, CookedShaderPackageFeatureFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasCookedShaderPackageFeature(CookedShaderPackageFeatureFlags value, CookedShaderPackageFeatureFlags flag) noexcept
{
	return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

enum class CookedShaderRayTracingExportKind : std::uint8_t
{
	None = 0,
	RayGeneration = 1,
	Miss = 2,
	ClosestHit = 3,
	AnyHit = 4,
	Intersection = 5,
	Callable = 6,
};

enum class CookedShaderRayTracingHitGroupType : std::uint8_t
{
	Triangles = 0,
	ProceduralPrimitive = 1,
};

enum class CookedShaderBinaryFormat : std::uint8_t
{
	Dxil = 0,
	SpirV = 1,
};

enum class CookedShaderSpecializationValueType : std::uint8_t
{
	Bool = 0,
	Int32 = 1,
	UInt32 = 2,
	Float32 = 3,
};

struct CookedShaderStringRef
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;

	constexpr bool IsValid() const noexcept { return SizeInBytes > 0; }
	explicit constexpr operator bool() const noexcept { return IsValid(); }
};

struct CookedShaderBlobRef
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;

	constexpr bool IsValid() const noexcept { return SizeInBytes > 0; }
	explicit constexpr operator bool() const noexcept { return IsValid(); }
};

struct CookedShaderPackageHeader
{
	std::uint32_t Magic = kCookedShaderPackageMagic;
	std::uint32_t Version = kCookedShaderPackageVersion;
	ShaderStageMask DeclaredStages = ShaderStageMask::None;
	std::uint8_t Reserved0[3] = {};
	std::uint16_t ShaderModelMajor = 0;
	std::uint16_t ShaderModelMinor = 0;
	std::uint32_t BinaryRecordCount = 0;
	std::uint32_t BindingRecordCount = 0;
	std::uint32_t PipelineLayoutRecordCount = 0;
	std::uint32_t SpecializationInputCount = 0;
	std::uint32_t StringTableSizeInBytes = 0;
	std::uint32_t BinaryBlobSizeInBytes = 0;
	// Reflection records are positional with binary records. Ray-tracing library
	// entries currently contain empty reflection ranges rather than omitting the
	// corresponding record.
	std::uint32_t ReflectionRecordCount = 0;
	std::uint32_t ResourceBindingRecordCount = 0;
	std::uint32_t ConstantBufferRecordCount = 0;
	std::uint32_t ConstantBufferMemberRecordCount = 0;
	std::uint32_t InputElementRecordCount = 0;
	std::uint32_t PushConstantRangeRecordCount = 0;
	std::uint32_t SpecializationConstantRecordCount = 0;
	CookedShaderPackageKind PackageKind = CookedShaderPackageKind::Graphics;
	std::uint8_t Reserved1[3] = {};
	CookedShaderPackageFeatureFlags PackageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint32_t RayTracingExportRecordCount = 0;
	std::uint32_t RayTracingHitGroupRecordCount = 0;
	std::uint32_t RayTracingLocalParameterRecordCount = 0;
	std::uint32_t RayTracingPayloadSizeInBytes = 0;
	std::uint32_t RayTracingAttributeSizeInBytes = 0;
	std::uint32_t RayTracingMaxRecursionDepth = 0;
	std::uint64_t ShaderPackageKey = 0;
	std::uint64_t SourceIdentityHash = 0;
	std::uint64_t BindingLayoutHash = 0;

	constexpr bool Matches(std::uint32_t expectedMagic, std::uint32_t expectedVersion) const noexcept
	{
		return Magic == expectedMagic && Version == expectedVersion;
	}
};

struct CookedShaderBinaryRecord
{
	std::uint64_t ShaderBlobId = 0;
	CookedShaderStringRef EntryPoint = {};
	CookedShaderStringRef ExportName = {};
	CookedShaderStringRef DebugArtifact = {};
	CookedShaderBlobRef Bytecode = {};
	ShaderStage Stage = ShaderStage::Count;
	CookedShaderBinaryFormat Format = CookedShaderBinaryFormat::Dxil;
	std::uint16_t Reserved = 0;
	std::uint32_t Flags = 0;
	std::uint64_t BytecodeHash = 0;
	// Producer identity. BackendName is interned in the package string table
	// ("dxc", "slang", ...). BackendVersion is the backend's reported version
	// stamp; renderers/tools must not parse it but may display it.
	CookedShaderStringRef BackendName = {};
	CookedShaderStringRef CodegenTarget = {};
	std::uint64_t BackendVersion = 0;
};

struct CookedShaderPipelineLayoutRecord
{
	CookedShaderStringRef CodegenTarget = {};
	std::uint64_t BindingLayoutHash = 0;
	std::uint32_t BindingRecordOffset = 0;
	std::uint32_t BindingRecordCount = 0;
	std::uint32_t DescriptorBindingCount = 0;
	std::uint32_t DescriptorSetCount = 0;
	std::uint32_t PushConstantRangeCount = 0;
	std::uint32_t PushConstantSizeInBytes = 0;
	std::uint32_t ConstantBufferCount = 0;
	std::uint32_t ReadOnlyResourceCount = 0;
	std::uint32_t ReadWriteResourceCount = 0;
	std::uint32_t SamplerCount = 0;
	std::uint32_t AccelerationStructureCount = 0;
	std::uint32_t Reserved = 0;
};

struct CookedShaderRayTracingExportRecord
{
	CookedShaderStringRef ExportName = {};
	CookedShaderStringRef EntryPoint = {};
	std::uint32_t BinaryRecordIndex = 0;
	CookedShaderRayTracingExportKind Kind = CookedShaderRayTracingExportKind::None;
	std::uint8_t Reserved0[3] = {};
	std::uint32_t Flags = 0;
	std::uint64_t ExportHash = 0;
};

struct CookedShaderRayTracingHitGroupRecord
{
	CookedShaderStringRef HitGroupName = {};
	CookedShaderRayTracingHitGroupType Type = CookedShaderRayTracingHitGroupType::Triangles;
	std::uint8_t Reserved0[3] = {};
	std::uint32_t ClosestHitExportIndex = UINT32_MAX;
	std::uint32_t AnyHitExportIndex = UINT32_MAX;
	std::uint32_t IntersectionExportIndex = UINT32_MAX;
	std::uint64_t HitGroupHash = 0;
};

struct CookedShaderRayTracingLocalParameterRecord
{
	CookedShaderStringRef Name = {};
	std::uint32_t OwnerExportIndex = UINT32_MAX;
	std::uint32_t BindingRecordOffset = 0;
	std::uint32_t BindingRecordCount = 0;
	std::uint64_t BindingLayoutHash = 0;
};

struct CookedShaderBindingRecord
{
	CookedShaderStringRef Name = {};
	ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
	std::uint32_t LogicalBindingIndex = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;
	std::uint32_t Reserved = 0;
};

struct CookedShaderSpecializationInputRecord
{
	CookedShaderStringRef Name = {};
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
	CookedShaderSpecializationValueType ValueType = CookedShaderSpecializationValueType::UInt32;
	std::uint16_t Reserved0 = 0;
	std::uint32_t LogicalIndex = 0;
	std::uint64_t DefaultValueBits = 0;
};

static_assert(std::is_trivially_copyable_v<CookedShaderStringRef>, "CookedShaderStringRef must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderBlobRef>, "CookedShaderBlobRef must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderPackageHeader>, "CookedShaderPackageHeader must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderBinaryRecord>, "CookedShaderBinaryRecord must stay trivially copyable.");
static_assert(
	std::is_trivially_copyable_v<CookedShaderPipelineLayoutRecord>,
	"CookedShaderPipelineLayoutRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<CookedShaderRayTracingExportRecord>,
    "CookedShaderRayTracingExportRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<CookedShaderRayTracingHitGroupRecord>,
    "CookedShaderRayTracingHitGroupRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<CookedShaderRayTracingLocalParameterRecord>,
    "CookedShaderRayTracingLocalParameterRecord must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderBindingRecord>, "CookedShaderBindingRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<CookedShaderSpecializationInputRecord>,
    "CookedShaderSpecializationInputRecord must stay trivially copyable.");
