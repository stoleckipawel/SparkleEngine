#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "FrameGraph/Features/FrameGraphProducts.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class CommandContext;
struct DeferredLightingPassRuntime;
class FrameGraph;
struct RenderGraphPassContext;
struct RenderPassContext;
struct RenderViewContext;

struct DeferredLightingPassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferEmissive;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;

	static void Describe(ShaderParameterStructBuilder<DeferredLightingPassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &DeferredLightingPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DeferredLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DeferredLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DeferredLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferEmissive", &DeferredLightingPassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &DeferredLightingPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DeferredLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DeferredLightingPassParameters::PerView, ShaderStageVisibility::Compute);
	}
};

class DeferredLightingPass final
{
  public:
	static constexpr const char* PassName = "DeferredLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DeferredLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
	    FrameGraph& frameGraph,
	    const SceneTargets& sceneTargets,
	    const GBufferTargets& gbuffer,
	    ParameterInstance& parameters);
	static void SetParameters(
	    ParameterInstance& parameters,
	    const RenderViewContext& viewContext,
	    const RenderPassContext& renderPassContext);
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);
};