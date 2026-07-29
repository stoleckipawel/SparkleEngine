#pragma once

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "RayTracing/RayTracingHitData.h"
#include "Scene/Meshes/MeshSkinningData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <vector>

struct MeshMorphTargetDelta;

struct GPUMeshPreparedData final
{
	ImmutableRenderMeshHandle Source;
	DirectX::XMFLOAT3 LocalBoundsMin = {};
	DirectX::XMFLOAT3 LocalBoundsMax = {};
	std::vector<RayTracingHitVertex> RayTracingVertices;
	std::vector<std::uint32_t> RayTracingIndices;
	std::vector<VertexSkinInfluence> SkinInfluences;
	std::vector<VertexSkinInfluenceData> GpuSkinInfluences;
	std::vector<MorphTargetDeltaData> MorphTargetDeltas;
	std::uint32_t MorphTargetCount = 0u;
	bool HasLocalBounds = false;

	bool IsValid() const noexcept;
	std::uint64_t GetDecodedByteSize() const noexcept;
	std::uint64_t GetResidentByteSize() const noexcept;
};

class GPUMeshPreparation final
{
  public:
	static bool Build(
	    const ImmutableRenderMeshHandle& source,
	    GPUMeshPreparedData& output);

  private:
	static void BuildBoundsAndRayTracing(
	    GPUMeshPreparedData& output);
	static void BuildSkinInfluences(
	    GPUMeshPreparedData& output);
	static bool BuildMorphTargets(
	    GPUMeshPreparedData& output);
	static VertexSkinInfluenceData ConvertSkinInfluence(
	    const VertexSkinInfluence& influence) noexcept;
	static MorphTargetDeltaData ConvertMorphTargetDelta(
	    const MeshMorphTargetDelta& delta) noexcept;
};
