#pragma once

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "Meshes/GpuMesh.h"
#include "Resources/Residency/AssetResidency.h"
#include "Tasks/Public/TaskExecution.h"

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

class Mesh;
class RenderCommandList;
class RenderHardwareInterface;
class RhiCommandSubmissionService;
class TaskExecutor;
class TaskScope;
struct GpuMeshPreparedData;

class GpuMeshCache final
{
public:
	GpuMeshCache(
	    RenderHardwareInterface& renderHardwareInterface,
	    RhiCommandSubmissionService& submissions,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationScope);
	~GpuMeshCache() noexcept;

	GpuMeshCache(const GpuMeshCache&) = delete;
	GpuMeshCache& operator=(const GpuMeshCache&) = delete;
	GpuMeshCache(GpuMeshCache&&) = delete;
	GpuMeshCache& operator=(GpuMeshCache&&) = delete;

	GpuMeshHandle Request(const ImmutableRenderMeshHandle& mesh);
	void UploadReadyMeshes(RenderCommandList& commandList);
	void RecordUploadSubmission(RhiSubmissionToken token) noexcept;
	void PollResidency() noexcept;
	void RetainOnly(std::span<const GpuMeshHandle> handles) noexcept;

	const GpuMesh* Resolve(GpuMeshHandle handle) const noexcept;

	std::size_t GetCachedCount() const noexcept;
	bool Contains(const Mesh& cpuMesh) const noexcept;
	const GpuMesh* Find(const Mesh& cpuMesh) const noexcept;

private:
	using CacheKey = std::pair<std::uint64_t, std::uint32_t>;

	struct ActiveMesh final
	{
		AssetGenerationHandle Generation;
		const Mesh* Source = nullptr;
		std::unique_ptr<GpuMesh> Mesh;
	};

	struct MeshRequest final
	{
		CacheKey Key;
		ImmutableRenderMeshHandle Source;
		GpuMeshHandle Handle;
		AssetGenerationHandle Generation;
		TaskExecution Execution;
		std::shared_ptr<GpuMeshPreparedData> Prepared;
		std::unique_ptr<GpuMesh> Uploaded;
		std::uint64_t ResidentBytes = 0;
		bool PreparationStarted = false;
		bool UploadSubmitted = false;
		bool Wanted = true;
	};

	static constexpr std::size_t kMaximumConcurrentPreparations = 16;

	GpuMeshHandle AllocateHandle();
	MeshRequest* FindRequest(const CacheKey& key) noexcept;
	void LaunchPendingPreparations();
	void LaunchPreparation(MeshRequest& request);
	void ConsumeCompletedPreparations() noexcept;
	void ActivateResidentMeshes() noexcept;
	void RemoveTerminalRequests() noexcept;
	void RetireActiveMesh(ActiveMesh& mesh) noexcept;
	RhiSubmissionState CaptureLastSubmittedState() const noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiCommandSubmissionService* m_submissions = nullptr;
	TaskExecutor* m_taskExecutor = nullptr;
	std::unique_ptr<TaskScope> m_taskScope;
	AssetResidency m_residency;
	std::uint64_t m_nextGpuMeshHandle = 1u;
	std::map<CacheKey, ActiveMesh> m_cache;
	std::unordered_map<std::uint64_t, const GpuMesh*> m_handles;
	std::unordered_map<const Mesh*, GpuMeshHandle> m_sourceHandles;
	std::map<CacheKey, MeshRequest> m_requests;
};
