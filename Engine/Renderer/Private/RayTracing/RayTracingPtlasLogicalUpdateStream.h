#pragma once

#include "RHI/Public/RayTracing/RhiPartitionedTlasDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <vector>

struct RayTracingPtlasPartitionEntry;
struct RayTracingPtlasPartitionPlan;
struct RenderSceneData;

struct RayTracingPtlasLogicalUpdateStreamResult final
{
	std::vector<RhiPartitionedTlasLogicalUpdateRecord> Records;
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
	static RhiPartitionedTlasLogicalUpdateFlags BuildUpdateFlags(const RayTracingPtlasPartitionEntry& entry) noexcept;
	static std::array<float, 12> BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
};
