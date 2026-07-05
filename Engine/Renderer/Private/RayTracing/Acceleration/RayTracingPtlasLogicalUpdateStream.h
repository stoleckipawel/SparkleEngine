#pragma once

#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <vector>

struct RayTracingPtlasPartitionEntry;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

struct RayTracingPtlasLogicalUpdateRecord final
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::uint32_t RenderInstanceIndex = 0;
	std::uint32_t InstanceIndex = 0;
	std::uint32_t PartitionIndex = 0;
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiPartitionedTlasInstanceFlags InstanceFlags = RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable;
};

struct RayTracingPtlasLogicalUpdateStreamResult final
{
	std::vector<RayTracingPtlasLogicalUpdateRecord> Records;
	std::uint32_t LogicalUpdateCount = 0;
	std::uint32_t SkippedInvalidInstanceCount = 0;
};

class RayTracingPtlasLogicalUpdateStream final
{
  public:
	RayTracingPtlasLogicalUpdateStreamResult Build(
	    const RenderSceneData& sceneData,
	    const RayTracingPtlasPartitionPlan& partitionPlan) const;

  private:
	static bool ShouldEmitLogicalUpdate(const RayTracingPtlasPartitionEntry& entry) noexcept;
	static std::array<float, 12> BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
};
