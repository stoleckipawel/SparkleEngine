#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingUniformData.h"
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
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderPassDefinition;
struct RenderViewData;

struct PathTracedIndirectLightingPassParameters
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
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderUniform<PathTracedLightingUniformData> PathTracedLightingConstants;

	static void Describe(ShaderParameterStructBuilder<PathTracedIndirectLightingPassParameters>& builder);
};

class PathTracedIndirectLightingPass final
{
  public:
	static constexpr const char* PassName = "PathTracedIndirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = PathTracedIndirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit PathTracedIndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	const ComputePassPipelineRuntime& m_runtime;
};
