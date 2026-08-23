#pragma once

#include "ShaderStage.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

// ShaderReflection — backend-neutral reflection metadata extracted from a
// compiled shader binary by the offline cooker and serialized into the shader
// map. Runtime modules consume it to build binding layouts and verify C++
// parameter declarations against the compiled shader ABI.
//
// The POD records are serialized directly. String references point into the
// map string table.

constexpr std::uint32_t kCookedShaderReflectionInvalidIndex = 0xFFFFFFFFu;

enum class CookedShaderResourceKind : std::uint8_t
{
	Unknown = 0,
	ConstantBuffer,
	Texture,
	StructuredBuffer,
	ByteAddressBuffer,
	TypedBuffer,
	RWTexture,
	RWStructuredBuffer,
	RWByteAddressBuffer,
	RWTypedBuffer,
	Sampler,
	AccelerationStructure,
	PushConstantBlock,
};

enum class CookedShaderResourceDimension : std::uint8_t
{
	Unknown = 0,
	Buffer,
	Texture1D,
	Texture1DArray,
	Texture2D,
	Texture2DArray,
	Texture2DMS,
	Texture2DMSArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
};

enum class CookedShaderScalarType : std::uint8_t
{
	Unknown = 0,
	Bool,
	Int32,
	UInt32,
	Float16,
	Float32,
	Int16,
	UInt16,
	Int64,
	UInt64,
	Float64,
};

// One resource binding declared by the shader. Slot/space follow the HLSL/
// SPIR-V binding model; both backends populate these in the same scheme:
// `Set` is `space` for HLSL register notation and `set` for SPIR-V; `Slot`
// is the register number / SPIR-V binding number.
struct CookedShaderResourceBindingRecord
{
	std::uint32_t NameOffsetInBytes = 0;
	std::uint32_t NameSizeInBytes = 0;
	CookedShaderResourceKind Kind = CookedShaderResourceKind::Unknown;
	CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
	std::uint8_t IsReadOnly = 0;
	std::uint8_t Reserved0 = 0;
	std::uint32_t Set = 0;
	std::uint32_t Slot = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t SizeInBytes = 0;
	// Index of the entry in the constant-buffer table when Kind ==
	// ConstantBuffer or PushConstantBlock; otherwise kCookedShaderReflectionInvalidIndex.
	std::uint32_t ConstantBufferIndex = kCookedShaderReflectionInvalidIndex;
};

struct CookedShaderConstantBufferMemberRecord
{
	std::uint32_t NameOffsetInBytes = 0;
	std::uint32_t NameSizeInBytes = 0;
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ArrayStrideInBytes = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
	std::uint8_t RowCount = 0;
	std::uint8_t ColumnCount = 0;
	std::uint8_t Reserved0 = 0;
};

// A constant buffer / push-constant block. Members are stored in a single
// flat array in the map; this record points to a contiguous range of it.
struct CookedShaderConstantBufferRecord
{
	std::uint32_t NameOffsetInBytes = 0;
	std::uint32_t NameSizeInBytes = 0;
	std::uint32_t MemberOffset = 0;
	std::uint32_t MemberCount = 0;
	std::uint32_t SizeInBytes = 0;
	std::uint32_t Padding = 0;
};

// One vertex input element (VS only). For other stages, InputElementCount
// is zero in the parent reflection record.
struct CookedShaderInputElementRecord
{
	std::uint32_t SemanticOffsetInBytes = 0;
	std::uint32_t SemanticSizeInBytes = 0;
	std::uint32_t SemanticIndex = 0;
	std::uint32_t Location = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
	std::uint8_t ComponentCount = 0;
	std::uint16_t Reserved = 0;
};

// Push constant range. DXIL emits these as root constants; SPIR-V emits a
// single push-constant block. Both shapes serialize as ranges with absolute
// offsets in the block.
struct CookedShaderPushConstantRangeRecord
{
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
	std::uint8_t Reserved0[3] = {};
};

// SPIR-V specialization constant (id, type, default value bits).
struct CookedShaderSpecializationConstantRecord
{
	std::uint32_t NameOffsetInBytes = 0;
	std::uint32_t NameSizeInBytes = 0;
	std::uint32_t ConstantId = 0;
	std::uint64_t DefaultValueBits = 0;
	CookedShaderScalarType ScalarType = CookedShaderScalarType::Unknown;
	std::uint8_t Reserved0[7] = {};
};

// Per-binary reflection block. One CookedShaderReflectionRecord exists for
// every map entry. Each record points to contiguous ranges in the map's typed
// reflection arrays.
struct CookedShaderReflectionRecord
{
	std::uint32_t ResourceBindingOffset = 0;
	std::uint32_t ResourceBindingCount = 0;
	std::uint32_t ConstantBufferOffset = 0;
	std::uint32_t ConstantBufferCount = 0;
	std::uint32_t InputElementOffset = 0;
	std::uint32_t InputElementCount = 0;
	std::uint32_t PushConstantRangeOffset = 0;
	std::uint32_t PushConstantRangeCount = 0;
	std::uint32_t SpecializationConstantOffset = 0;
	std::uint32_t SpecializationConstantCount = 0;
	std::uint32_t ThreadGroupSize[3] = {0, 0, 0};
	std::uint32_t EntryFlags = 0;
	std::uint16_t WaveSize = 0;
	std::uint16_t Reserved = 0;
};

static_assert(std::is_trivially_copyable_v<CookedShaderResourceBindingRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderConstantBufferRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderConstantBufferMemberRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderInputElementRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderPushConstantRangeRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderSpecializationConstantRecord>, "must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedShaderReflectionRecord>, "must stay trivially copyable.");
