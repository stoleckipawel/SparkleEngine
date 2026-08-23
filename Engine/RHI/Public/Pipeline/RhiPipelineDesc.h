#pragma once

#include "../Formats/CompareOp.h"
#include "../Formats/PixelFormat.h"
#include "../RHIAPI.h"
#include "../Resources/RhiResourceDesc.h"
#include "../ShaderParameters/ShaderParameterSemantics.h"
#include "../Shaders/ShaderStage.h"
#include "../Shaders/GlobalShaderMap.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <span>
#include <vector>

class PassParameterLayout;

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

enum class RhiFillMode : std::uint8_t
{
	Solid = 0,
	Wireframe = 1,
};

enum class RhiBlendFactor : std::uint8_t
{
	Zero = 0,
	One = 1,
	SourceAlpha = 2,
	InverseSourceAlpha = 3,
};

enum class RhiBlendOperation : std::uint8_t
{
	Add = 0,
};

enum class RhiVertexElementFormat : std::uint8_t
{
	Float2 = 0,
	Float3 = 1,
	Float4 = 2,
};

enum class RhiVertexSemantic : std::uint8_t
{
	Position = 0,
	TexCoord = 1,
	Normal = 2,
	Tangent = 3,
};

enum class CompiledBindingType : std::uint8_t
{
	ConstantBuffer,
	ReadOnlyAddress,
	ReadWriteAddress,
	AccelerationStructure,
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
	std::span<const ResolvedShader> Shaders;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* DebugName = L"RHI_BindingLayout";
	bool InlineUniformDataAsPushConstants = false;
};

class SPARKLE_RHI_API RenderBindingLayout
{
public:
	virtual ~RenderBindingLayout() noexcept;

	const PassParameterLayout& GetParameterLayout() const noexcept;
	const CompiledBinding* GetBindings() const noexcept;
	std::size_t GetBindingCount() const noexcept;
	const CompiledBinding* FindBinding(const char* name) const noexcept;

protected:
	RenderBindingLayout(
	    const PassParameterLayout& parameterLayout,
	    std::vector<CompiledBinding> bindings,
	    std::vector<std::string> bindingNames) noexcept;

private:
	const PassParameterLayout* m_parameterLayout = nullptr;
	std::vector<CompiledBinding> m_bindings;
	std::vector<std::string> m_bindingNames;
};

struct RhiBlendTargetState
{
	bool BlendEnable = false;
	RhiBlendFactor SourceColor = RhiBlendFactor::One;
	RhiBlendFactor DestinationColor = RhiBlendFactor::Zero;
	RhiBlendOperation ColorOperation = RhiBlendOperation::Add;
	RhiBlendFactor SourceAlpha = RhiBlendFactor::One;
	RhiBlendFactor DestinationAlpha = RhiBlendFactor::Zero;
	RhiBlendOperation AlphaOperation = RhiBlendOperation::Add;
	std::uint8_t ColorWriteMask = 0x0F;

	bool operator==(const RhiBlendTargetState&) const noexcept = default;
};

struct RhiBlendState
{
	bool AlphaToCoverageEnable = false;
	bool IndependentBlendEnable = false;
	std::array<RhiBlendTargetState, 8> Targets = {};

	bool operator==(const RhiBlendState&) const noexcept = default;
};

struct RhiRasterizerState
{
	RhiFillMode FillMode = RhiFillMode::Solid;
	ERhiCullMode CullMode = ERhiCullMode::Back;
	ERhiFrontFaceWinding FrontFaceWinding = ERhiFrontFaceWinding::Clockwise;
	bool DepthClipEnable = true;

	bool operator==(const RhiRasterizerState&) const noexcept = default;
};

struct RhiDepthState
{
	bool DepthEnable = true;
	bool DepthWriteEnable = true;
	CompareOp DepthFunc = CompareOp::Less;

	bool operator==(const RhiDepthState&) const noexcept = default;
};

struct RhiStencilState
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

	bool operator==(const RhiStencilState&) const noexcept = default;
};

struct RhiVertexInputBinding
{
	std::uint32_t Binding = 0;
	std::uint32_t StrideInBytes = 0;
	bool PerInstance = false;

	bool operator==(const RhiVertexInputBinding&) const noexcept = default;
};

struct RhiVertexInputElement
{
	RhiVertexSemantic Semantic = RhiVertexSemantic::Position;
	std::uint8_t SemanticIndex = 0;
	std::uint8_t Location = 0;
	std::uint8_t Binding = 0;
	RhiVertexElementFormat Format = RhiVertexElementFormat::Float3;
	std::uint32_t OffsetInBytes = 0;

	bool operator==(const RhiVertexInputElement&) const noexcept = default;
};

struct RhiVertexInputDeclaration
{
	std::array<RhiVertexInputBinding, 4> Bindings = {};
	std::array<RhiVertexInputElement, 16> Elements = {};
	std::uint8_t BindingCount = 0;
	std::uint8_t ElementCount = 0;

	bool operator==(const RhiVertexInputDeclaration&) const noexcept = default;
};

struct RhiShaderStageDesc
{
	const ResolvedShader* Shader = nullptr;

	bool IsValid() const noexcept { return Shader != nullptr && Shader->IsValid(); }
	explicit operator bool() const noexcept { return IsValid(); }
};

struct GraphicsPipelineDesc
{
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc VertexShader = {};
	RhiShaderStageDesc PixelShader = {};
	RhiBlendState Blend = {};
	RhiRasterizerState Rasterizer = {};
	RhiDepthState Depth = {};
	RhiStencilState Stencil = {};
	RhiPrimitiveTopology PrimitiveTopology = RhiPrimitiveTopology::TriangleList;
	RhiVertexInputDeclaration VertexInput = {};
	std::array<PixelFormat, 8> ColorAttachmentFormats = {};
	std::uint32_t ColorAttachmentCount = 0;
	PixelFormat DepthStencilAttachmentFormat = PixelFormat::Unknown;
	std::uint8_t SampleCount = 1;
	const wchar_t* DebugName = L"RHI_GraphicsPipeline";
};

struct ComputePipelineDesc
{
	const RenderBindingLayout* BindingLayout = nullptr;
	RhiShaderStageDesc ComputeShader = {};
	const wchar_t* DebugName = L"RHI_ComputePipeline";
};

class SPARKLE_RHI_API RenderPipeline
{
public:
	virtual ~RenderPipeline() noexcept = default;
};
