#pragma once

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "Meshes/GPUMesh.h"
#include "Resources/Residency/AssetResidency.h"
#include "Tasks/Public/TaskExecution.h"

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Mesh;
class RenderCommandList;
class RenderHardwareInterface;
class RhiCommandSubmissionService;
class TaskExecutor;
class TaskScope;
struct GPUMeshPreparedData;

class GPUMeshCache final
{
  public:
	GPUMeshCache(
	    RenderHardwareInterface& renderHardwareInterface,
	    RhiCommandSubmissionService& submissions,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationScope);
	~GPUMeshCache() noexcept;

	GPUMeshCache(const GPUMeshCache&) = delete;
	GPUMeshCache& operator=(const GPUMeshCache&) = delete;
	GPUMeshCache(GPUMeshCache&&) = delete;
	GPUMeshCache& operator=(GPUMeshCache&&) = delete;

	GpuMeshHandle Request(const ImmutableRenderMeshHandle& mesh);
	void UploadReadyMeshes(
	    RenderCommandList& commandList);
	void RecordUploadSubmission(RhiSubmissionToken token) noexcept;
	void PollResidency() noexcept;
	void RetainOnly(std::span<const GpuMeshHandle> handles) noexcept;

	const GPUMesh* Resolve(GpuMeshHandle handle) const noexcept;
	bool HasFailed(GpuMeshHandle handle) const noexcept;

	std::size_t GetCachedCount() const noexcept;
	bool Contains(const Mesh& cpuMesh) const noexcept;
	const GPUMesh* Find(const Mesh& cpuMesh) const noexcept;

  private:
	using CacheKey = std::pair<std::uint64_t, std::uint32_t>;

	struct ActiveMesh final
	{
		AssetGenerationHandle Generation;
		const Mesh* Source = nullptr;
		std::unique_ptr<GPUMesh> Mesh;
	};

	struct MeshRequest final
	{
		CacheKey Key;
		ImmutableRenderMeshHandle Source;
		GpuMeshHandle Handle;
		AssetGenerationHandle Generation;
		TaskExecution Execution;
		std::shared_ptr<GPUMeshPreparedData> Prepared;
		std::unique_ptr<GPUMesh> Uploaded;
		std::uint64_t ResidentBytes = 0;
		bool UploadSubmitted = false;
		bool Wanted = true;
	};

	GpuMeshHandle AllocateHandle() noexcept;
	MeshRequest* FindRequest(const CacheKey& key) noexcept;
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
	std::unordered_map<std::uint64_t, const GPUMesh*> m_handles;
	std::unordered_map<const Mesh*, GpuMeshHandle> m_sourceHandles;
	std::unordered_set<std::uint64_t> m_failedHandles;
	std::vector<MeshRequest> m_requests;
};
