#pragma once

#include "SceneData/GpuScene/PersistentStructuredBuffer.h"
#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

#include <cstdint>
#include <vector>

struct MeshDraw;
struct MeshInstanceData;
struct RenderSceneData;

class RenderGpuGeometryState final
{
  public:
	void Update(const RenderSceneData& sceneData);
	void CollectMeshInstanceWriteRanges(
	    std::uint64_t appliedRevision,
	    std::vector<StructuredBufferElementRange>& ranges) const;
	void Reset() noexcept;

	const RenderGpuGeometryPayloads& GetPayloads() const noexcept { return m_payloads; }
	std::uint64_t GetMeshInstanceRevision() const noexcept { return m_meshInstanceRevision; }
	std::uint64_t GetMeshInstanceSlotRevision() const noexcept { return m_meshInstanceSlotRevision; }

  private:
	void UpdateMeshInstances(const RenderSceneData& sceneData);
	void UpdateMeshInstanceSlots(const RenderSceneData& sceneData);
	void UpdateDeformation(const RenderSceneData& sceneData);
	static MeshInstanceData BuildMeshInstance(const MeshDraw& draw) noexcept;
	static bool HasSameMeshInstance(
	    const MeshInstanceData& left,
	    const MeshInstanceData& right) noexcept;

	RenderGpuGeometryPayloads m_payloads;
	std::vector<std::uint64_t> m_meshInstanceElementRevisions;
	std::vector<std::uint32_t> m_meshInstanceSlotScratch;
	std::uint64_t m_meshInstanceRevision = 0u;
	std::uint64_t m_meshInstanceSlotRevision = 0u;
};
