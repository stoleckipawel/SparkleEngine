#pragma once

#include "Scene/GpuScene/PersistentStructuredBuffer.h"
#include "Scene/GpuScene/RenderGpuScenePayloads.h"

#include <cstdint>
#include <vector>

struct MeshDraw;
struct MeshInstanceData;
struct PreparedRenderScene;
struct RenderView;

class RenderGpuGeometryState final
{
public:
	void Update(const PreparedRenderScene& preparedScene, const RenderView& view);
	void CollectMeshInstanceWriteRanges(std::uint64_t appliedRevision, std::vector<StructuredBufferElementRange>& ranges) const;
	void Reset() noexcept;

	const RenderGpuGeometryPayloads& GetPayloads() const noexcept { return m_payloads; }
	std::uint64_t GetMeshInstanceRevision() const noexcept { return m_meshInstanceRevision; }
	std::uint64_t GetMeshInstanceSlotRevision() const noexcept { return m_meshInstanceSlotRevision; }

private:
	void UpdateMeshInstances(const PreparedRenderScene& preparedScene);
	void UpdateMeshInstanceSlots(const PreparedRenderScene& preparedScene, const RenderView& view);
	void UpdateDeformation(const PreparedRenderScene& preparedScene);
	static MeshInstanceData BuildMeshInstance(const MeshDraw& draw) noexcept;
	static bool HasSameMeshInstance(const MeshInstanceData& left, const MeshInstanceData& right) noexcept;

	RenderGpuGeometryPayloads m_payloads;
	std::vector<std::uint64_t> m_meshInstanceElementRevisions;
	std::vector<std::uint32_t> m_meshInstanceSlotScratch;
	std::uint64_t m_meshInstanceRevision = 0u;
	std::uint64_t m_meshInstanceSlotRevision = 0u;
};
