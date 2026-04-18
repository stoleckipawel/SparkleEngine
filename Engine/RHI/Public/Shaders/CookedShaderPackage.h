#pragma once

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
constexpr std::uint32_t kCookedShaderPackageVersion = 1;

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
	std::uint32_t SpecializationInputCount = 0;
	std::uint32_t StringTableSizeInBytes = 0;
	std::uint32_t BinaryBlobSizeInBytes = 0;
	std::uint32_t Reserved1 = 0;
	std::uint64_t ShaderPackageKey = 0;
	std::uint64_t SourceIdentityHash = 0;
	std::uint64_t BindingLayoutHash = 0;
	std::uint64_t VariantHash = 0;

	constexpr bool Matches(std::uint32_t expectedMagic, std::uint32_t expectedVersion) const noexcept
	{
		return Magic == expectedMagic && Version == expectedVersion;
	}
};

struct CookedShaderBinaryRecord
{
	CookedShaderStringRef EntryPoint = {};
	CookedShaderStringRef DebugArtifact = {};
	CookedShaderBlobRef Bytecode = {};
	ShaderStage Stage = ShaderStage::Count;
	CookedShaderBinaryFormat Format = CookedShaderBinaryFormat::Dxil;
	std::uint16_t Reserved = 0;
	std::uint32_t Flags = 0;
	std::uint64_t BytecodeHash = 0;
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
static_assert(std::is_trivially_copyable_v<CookedShaderBindingRecord>, "CookedShaderBindingRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<CookedShaderSpecializationInputRecord>,
    "CookedShaderSpecializationInputRecord must stay trivially copyable.");