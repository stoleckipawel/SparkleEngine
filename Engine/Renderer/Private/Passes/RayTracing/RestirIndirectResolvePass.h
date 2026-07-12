#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct RestirIndirectResolvePassParameters
{
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> SceneDepth;
	ShaderTexture2DSRV SkyTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderBufferSRV RayTracingHitVertices;
	ShaderBufferSRV RayTracingHitIndices;
	ShaderBufferSRV RayTracingHitInstances;
	ShaderBufferSRV RayTracingHitMaterials;
	ShaderBufferSRV MeshInstances;
	ShaderBufferSRV SkinInfluences;
	ShaderBufferSRV JointMatrices;
	ShaderBufferSRV DirectionalLights;
	ShaderBufferSRV PointLights;
	ShaderBufferSRV SpotLights;
	ShaderBufferSRV RectLights;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;
	ShaderUniform<RestirIndirectLightingUniformData> RestirIndirectConstants;
	ShaderTexture2D<void> CurrentReservoirSampleTexture;
	ShaderTexture2D<void> CurrentReservoirWeightTexture;
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderRWTexture2D<void> RayReconstructionDiffuseAlbedo;
	ShaderRWTexture2D<void> RayReconstructionSpecularAlbedo;
	ShaderRWTexture2D<void> RayReconstructionRoughness;
	ShaderRWTexture2D<void> RayReconstructionSpecularHitDistance;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectResolvePassParameters>& builder);
};

class RestirIndirectResolvePass final
{
  public:
	explicit RestirIndirectResolvePass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}
	static constexpr const char* PassName = "RestirIndirectResolve";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RestirIndirectResolvePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const SceneRenderTargets& scene,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphTextureHandle currentSample,
	    FrameGraphTextureHandle currentWeight,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};
