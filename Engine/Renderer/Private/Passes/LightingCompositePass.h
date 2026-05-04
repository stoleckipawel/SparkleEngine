#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class FrameGraph;
struct LightingCompositePassRuntime;
struct RenderGraphPassContext;
struct RenderPassContext;
struct RenderViewContext;

struct LightingCompositePassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> DirectDiffuse;
	ShaderTexture2D<void> DirectSpecular;
	ShaderTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> IndirectDiffuse;
	ShaderTexture2D<void> IndirectSpecular;
	ShaderTexture2D<void> IndirectSubsurface;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferEmissive;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;

	static void Describe(ShaderParameterStructBuilder<LightingCompositePassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &LightingCompositePassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectDiffuse", &LightingCompositePassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSpecular", &LightingCompositePassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSubsurface", &LightingCompositePassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectDiffuse", &LightingCompositePassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectSpecular", &LightingCompositePassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectSubsurface", &LightingCompositePassParameters::IndirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &LightingCompositePassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &LightingCompositePassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &LightingCompositePassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferEmissive", &LightingCompositePassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &LightingCompositePassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &LightingCompositePassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &LightingCompositePassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &LightingCompositePassParameters::PerView, ShaderStageVisibility::Compute);
	}
};

class LightingCompositePass final
{
  public:
	static constexpr const char* PassName = "LightingComposite";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = LightingCompositePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
		FrameGraph& frameGraph,
		const SceneTargets& sceneTargets,
		const LightingTargets& lighting,
		const GBufferTargets& gbuffer,
		ParameterInstance& parameters);
	static void SetParameters(
		ParameterInstance& parameters,
		const RenderViewContext& viewContext,
		const RenderPassContext& renderPassContext);
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);
};