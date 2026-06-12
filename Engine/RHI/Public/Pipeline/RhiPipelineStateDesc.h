#pragma once

#include "../Formats/CompareOp.h"
#include "../Formats/PixelFormat.h"
#include "../RHIAPI.h"
#include "../ShaderParameters/ShaderParameterSemantics.h"
#include "../Shaders/ShaderStage.h"

#include <array>
#include <cstddef>
#include <cstdint>

class PassParameterLayout;
class LoadedShaderPackage;

enum class ERhiCullMode : std::uint8_t
{
	None = 0,
	Front = 1,
	Back = 2,
};

enum class ERhiFrontFaceWinding : std::uint8_t
{
	Clockwise = 0,
	CounterClockwise = 1,
};

enum class RhiStencilOp : std::uint8_t
{
	Keep = 0,
	Zero = 1,
	Replace = 2,
	IncrementClamp = 3,
	DecrementClamp = 4,
	Invert = 5,
	IncrementWrap = 6,
	DecrementWrap = 7,
};

enum class RhiVertexLayoutKind : std::uint8_t
{
	StaticMesh = 0,
};

enum class CompiledBindingType : std::uint8_t
{
	ConstantBuffer,
	ReadOnlyAddress,
	ReadWriteAddress,
	ReadOnlyResourceTable,
	ReadWriteResourceTable,
	SamplerTable,
	PushConstants,
};

struct RhiBindingPoint
{
	// HLSL register space and SPIR-V descriptor set share Set; HLSL register and SPIR-V binding share Binding.
	std::uint32_t Set = 0;
	std::uint32_t Binding = 0;
};

struct RhiBindlessBindingMetadata
{
	// Reserved for descriptor-indexing/bindless layout compilation; bindful paths keep these fields disabled.
	bool BindlessEligible = false;
	bool RuntimeSizedArray = false;
	std::uint32_t ReservedDescriptorCount = 0;
};

struct CompiledBinding
{
	const char* Name = nullptr;
	CompiledBindingType Type = CompiledBindingType::ReadOnlyResourceTable;
	ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
	std::uint32_t BindingIndex = 0;
	RhiBindingPoint BindingPoint = {};
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
	std::uint32_t DescriptorCount = 0;
	std::uint32_t PushConstantCount = 0;
	RhiBindlessBindingMetadata Bindless = {};
};

struct RenderBindingLayoutCompileDesc
{
	const PassParameterLayout* ParameterLayout = nullptr;
	const LoadedShaderPackage* ShaderPackage = nullptr;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* DebugName = L"RHI_BindingLayout";
	bool InlineUniformDataAsPushConstants = false;
};

class SPARKLE_RHI_API RenderBindingLayout
{
  public:
	virtual ~RenderBindingLayout() noexcept = default;

	virtual const PassParameterLayout& GetParameterLayout() const noexcept = 0;
	virtual const CompiledBinding* GetBindings() const noexcept = 0;
	virtual std::size_t GetBindingCount() const noexcept = 0;
	virtual const CompiledBinding* FindBinding(const char* name) const noexcept = 0;
};

struct RhiDepthTestDesc
{
	bool DepthEnable = true;
	bool DepthWriteEnable = true;
	CompareOp DepthFunc = CompareOp::Less;
};

struct RhiStencilTestDesc
{
	bool StencilEnable = false;
	std::uint8_t StencilReadMask = 0xFF;
	std::uint8_t StencilWriteMask = 0xFF;
	CompareOp FrontFaceStencilFunc = CompareOp::Always;
	RhiStencilOp FrontFaceStencilFailOp = RhiStencilOp::Keep;
	RhiStencilOp FrontFaceStencilDepthFailOp = RhiStencilOp::Keep;
	RhiStencilOp FrontFaceStencilPassOp = RhiStencilOp::Keep;
	CompareOp BackFaceStencilFunc = CompareOp::Always;
	RhiStencilOp BackFaceStencilFailOp = RhiStencilOp::Keep;
	RhiStencilOp BackFaceStencilDepthFailOp = RhiStencilOp::Keep;
	RhiStencilOp BackFaceStencilPassOp = RhiStencilOp::Keep;
};

struct RhiShaderStageDesc
{
	const LoadedShaderPackage* Package = nullptr;
	ShaderStage Stage = ShaderStage::Count;

	constexpr bool IsValid() const noexcept { return Package != nullptr && Stage != ShaderStage::Count; }
	explicit constexpr operator bool() const noexcept { return IsValid(); }
};

struct GraphicsPipelineStateDesc
{
	RhiVertexLayoutKind VertexLayout = RhiVertexLayoutKind::StaticMesh;
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc VertexShader = {};
	RhiShaderStageDesc PixelShader = {};
	bool RenderWireframe = false;
	ERhiCullMode CullMode = ERhiCullMode::Back;
	ERhiFrontFaceWinding FrontFaceWinding = ERhiFrontFaceWinding::Clockwise;
	RhiDepthTestDesc DepthTest = {};
	RhiStencilTestDesc StencilTest = {};
	std::array<PixelFormat, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 1;
	PixelFormat DepthStencilFormat = PixelFormat::Unknown;
	const wchar_t* DebugName = L"RHI_GraphicsPipelineState";
};

struct ComputePipelineStateDesc
{
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc ComputeShader = {};
	const wchar_t* DebugName = L"RHI_ComputePipelineState";
};

class SPARKLE_RHI_API RenderPipelineState
{
  public:
	virtual ~RenderPipelineState() noexcept = default;
};
