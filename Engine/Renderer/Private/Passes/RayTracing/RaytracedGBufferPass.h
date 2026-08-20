#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"

#include <cstdint>
#include <type_traits>

struct ComputePassPipelineRuntime;
struct FrameContext;
struct PassExecutionContext;
struct PassRuntimeContext;
struct RenderPassDefinition;
struct RenderView;

struct RaytracedGBufferUniformData
{
	std::uint32_t RayTracingHitInstanceCount = 0u;
	std::uint32_t RayTracingHitMaterialCount = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
};

static_assert(std::is_standard_layout_v<RaytracedGBufferUniformData>, "RaytracedGBufferUniformData must be standard-layout");
static_assert(std::is_trivially_copyable_v<RaytracedGBufferUniformData>, "RaytracedGBufferUniformData must be trivially copyable");
static_assert(sizeof(RaytracedGBufferUniformData) == 16, "RaytracedGBufferUniformData must match the shader layout");

struct RaytracedGBufferPassParameters
{
	ShaderRWTexture2D<void> GBufferBaseColor;
	ShaderRWTexture2D<void> GBufferNormal;
	ShaderRWTexture2D<void> GBufferMaterial;
	ShaderRWTexture2D<void> GBufferEmissive;
	ShaderRWTexture2D<void> GBufferSubsurface;
	ShaderRWTexture2D<void> GBufferDeviceZ;
	ShaderRWTexture2D<void> GBufferMotionVector;
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<FrameUniformData> Frame;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<RaytracedGBufferUniformData> RaytracedGBufferConstants;
	ShaderBuffer<void> RayTracingHitVertices;
	ShaderBuffer<void> MorphTargetDeltas;
	ShaderBuffer<void> RayTracingHitIndices;
	ShaderBuffer<void> RayTracingHitInstances;
	ShaderBuffer<void> RayTracingHitMaterials;
	ShaderBuffer<void> MeshInstances;
	ShaderBuffer<void> SkinInfluences;
	ShaderBuffer<void> JointMatrices;
	ShaderBuffer<void> PreviousJointMatrices;
	ShaderBuffer<void> MorphWeights;
	ShaderBuffer<void> PreviousMorphWeights;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;

	static void Describe(ShaderParameterStructBuilder<RaytracedGBufferPassParameters>& builder);
};

class RaytracedGBufferPass final
{
public:
	static constexpr const char* PassName = "RaytracedGBuffer";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RaytracedGBufferPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit RaytracedGBufferPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderView& view,
	    const PassRuntimeContext& passRuntimeContext) const;

	const ComputePassPipelineRuntime& m_runtime;
};
