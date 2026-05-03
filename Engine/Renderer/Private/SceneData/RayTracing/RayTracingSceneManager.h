#pragma once

#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class CommandContext;
class GPUMesh;
struct MeshDraw;
struct RenderSceneData;

struct RayTracingSceneDiagnostics
{
	std::uint32_t bottomLevelCount = 0;
	std::uint32_t topLevelInstanceCount = 0;
	std::uint32_t rebuiltBottomLevelCount = 0;
	bool rebuiltTopLevel = false;
};

class RayTracingSceneManager final
{
  public:
	RayTracingSceneManager() = default;
	~RayTracingSceneManager() noexcept;

	RayTracingSceneManager(const RayTracingSceneManager&) = delete;
	RayTracingSceneManager& operator=(const RayTracingSceneManager&) = delete;
	RayTracingSceneManager(RayTracingSceneManager&&) = delete;
	RayTracingSceneManager& operator=(RayTracingSceneManager&&) = delete;

	void Update(RenderHardwareInterface& renderHardwareInterface, CommandContext& commandContext, const RenderSceneData& sceneData);
	void Reset() noexcept;

	RhiGpuVirtualAddress GetTopLevelAccelerationStructureGpuAddress() const noexcept { return m_topLevelGpuAddress; }
	const RayTracingSceneDiagnostics& GetDiagnostics() const noexcept { return m_diagnostics; }

  private:
	struct BottomLevelRecord
	{
		RhiOwnedResourceHandle result = {};
		RhiOwnedResourceHandle scratch = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		RhiRayTracingGeometryDesc geometry = {};
		std::uint64_t resultSizeInBytes = 0;
	};

	bool EnsureBottomLevel(
	    RenderHardwareInterface& renderHardwareInterface,
	    CommandContext& commandContext,
	    const GPUMesh& gpuMesh);
	void RebuildTopLevel(
	    RenderHardwareInterface& renderHardwareInterface,
	    CommandContext& commandContext,
	    const RenderSceneData& sceneData);
	void ReleaseBottomLevelRecord(RenderHardwareInterface& renderHardwareInterface, BottomLevelRecord& record) noexcept;
	void ReleaseTopLevelResources(RenderHardwareInterface& renderHardwareInterface) noexcept;
	static RhiRayTracingInstanceDesc BuildInstanceDesc(const MeshDraw& draw, RhiGpuVirtualAddress bottomLevelGpuAddress, std::uint32_t instanceId) noexcept;
	static bool GeometryMatches(const RhiRayTracingGeometryDesc& lhs, const RhiRayTracingGeometryDesc& rhs) noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::unordered_map<const GPUMesh*, BottomLevelRecord> m_bottomLevelRecords;
	RhiOwnedResourceHandle m_topLevelResult = {};
	RhiOwnedResourceHandle m_topLevelScratch = {};
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiGpuVirtualAddress m_topLevelGpuAddress = 0;
	std::uint64_t m_topLevelResultSizeInBytes = 0;
	std::uint64_t m_topLevelScratchSizeInBytes = 0;
	RayTracingSceneDiagnostics m_diagnostics = {};
};