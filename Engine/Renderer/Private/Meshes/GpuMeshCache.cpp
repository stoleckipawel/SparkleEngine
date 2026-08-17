#include "PCH.h"
#include "Meshes/GpuMeshCache.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMeshPreparation.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <utility>

static const auto g_gpuMeshCacheLogger = Logging::GetOrCreateLogger("Renderer.GpuMeshCache");

GpuMeshCache::GpuMeshCache(
    RenderHardwareInterface& renderHardwareInterface,
    RhiCommandSubmissionService& submissions,
    TaskExecutor& taskExecutor,
    TaskScope& applicationScope) :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_submissions(&submissions),
    m_taskExecutor(&taskExecutor),
    m_taskScope(std::make_unique<TaskScope>(TaskScopeDesc{TaskScopeKind::AssetGeneration, "Renderer mesh generations"}, &applicationScope))
{
}

GpuMeshCache::~GpuMeshCache() noexcept
{
	if (m_taskScope != nullptr)
	{
		m_taskScope->Cancel();
		(void) m_taskScope->JoinFor(std::chrono::milliseconds::max());
	}

	m_requests.clear();
	m_sourceHandles.clear();
	m_handles.clear();
	m_cache.clear();
}

GpuMeshHandle GpuMeshCache::Request(const ImmutableRenderMeshHandle& mesh)
{
	if (!mesh.IsValid())
	{
		Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh cache received an invalid mesh handle.");
	}

	const CacheKey key{mesh.GetAssetId(), mesh.GetGeneration()};
	const auto active = m_cache.find(key);
	if (active != m_cache.end())
	{
		return active->second.Mesh->GetHandle();
	}

	MeshRequest* const existingRequest = FindRequest(key);
	if (existingRequest != nullptr)
	{
		if (!existingRequest->Wanted)
		{
			return {};
		}
		existingRequest->Wanted = true;
		return existingRequest->Handle;
	}

	if (m_taskScope == nullptr)
	{
		Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh cache has no task scope.");
	}

	const std::size_t maximumRequestCount = m_residency.GetBudget().MaximumRequestBacklog;
	if (m_requests.size() >= maximumRequestCount)
	{
		RemoveTerminalRequests();
	}
	if (m_requests.size() >= maximumRequestCount || !m_residency.HasRequestCapacity())
	{
		return {};
	}

	auto prepared = std::make_shared<GpuMeshPreparedData>();
	const std::optional<AssetGenerationHandle> generation = m_residency.BeginGeneration(key.first, key.second);
	if (!generation)
	{
		return {};
	}
	const GpuMeshHandle handle = AllocateHandle();

	MeshRequest request;
	request.Key = key;
	request.Source = mesh;
	request.Handle = handle;
	request.Generation = *generation;
	request.Prepared = prepared;
	try
	{
		if (!m_requests.emplace(key, std::move(request)).second)
		{
			Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh request was admitted twice.");
		}
	}
	catch (...)
	{
		(void) m_residency.Cancel(*generation);
		throw;
	}
	LaunchPendingPreparations();
	return handle;
}

void GpuMeshCache::UploadReadyMeshes(RenderCommandList& commandList)
{
	ConsumeCompletedPreparations();
	LaunchPendingPreparations();

	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		if (!request.Wanted || request.Prepared == nullptr || request.Uploaded != nullptr
		    || m_residency.GetState(request.Generation) != AssetResidencyState::ReadyForUpload
		    || !m_residency.BeginUpload(request.Generation))
		{
			continue;
		}

		request.ResidentBytes = request.Prepared->GetResidentByteSize();
		auto gpuMesh = std::make_unique<GpuMesh>(request.Handle);
		gpuMesh->Upload(*m_renderHardwareInterface, commandList, std::move(*request.Prepared));

		request.Prepared.reset();
		request.Uploaded = std::move(gpuMesh);
	}
}

void GpuMeshCache::RecordUploadSubmission(RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		for (const auto& entry : m_requests)
		{
			const MeshRequest& request = entry.second;
			if (request.Uploaded != nullptr && !request.UploadSubmitted)
			{
				Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh upload completed without a submission token.");
			}
		}
		return;
	}

	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		if (request.Uploaded != nullptr && !request.UploadSubmitted)
		{
			if (!m_residency.RecordUploadSubmission(request.Generation, token, request.ResidentBytes))
			{
				Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh upload submission could not enter residency.");
			}
			request.UploadSubmitted = true;
		}
	}
}

void GpuMeshCache::PollResidency() noexcept
{
	ConsumeCompletedPreparations();
	m_residency.Poll(*m_submissions);
	ActivateResidentMeshes();
	RemoveTerminalRequests();
}

void GpuMeshCache::RetainOnly(std::span<const GpuMeshHandle> handles) noexcept
{
	std::vector<std::uint64_t> wantedHandles;
	wantedHandles.reserve(handles.size());
	for (GpuMeshHandle handle : handles)
	{
		if (handle)
		{
			wantedHandles.push_back(handle.Value);
		}
	}

	std::sort(wantedHandles.begin(), wantedHandles.end());
	wantedHandles.erase(std::unique(wantedHandles.begin(), wantedHandles.end()), wantedHandles.end());

	const auto isWanted = [&wantedHandles](GpuMeshHandle handle) noexcept
	{
		return std::binary_search(wantedHandles.begin(), wantedHandles.end(), handle.Value);
	};

	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		request.Wanted = isWanted(request.Handle);
		if (!request.Wanted)
		{
			(void) m_residency.Cancel(request.Generation);
			if (!request.Execution.IsValid() && request.Uploaded == nullptr)
			{
				request.Prepared.reset();
			}
		}
	}

	for (auto mesh = m_cache.begin(); mesh != m_cache.end();)
	{
		const GpuMeshHandle handle = mesh->second.Mesh->GetHandle();
		if (isWanted(handle))
		{
			++mesh;
			continue;
		}

		const auto sourceHandle = m_sourceHandles.find(mesh->second.Source);
		if (sourceHandle != m_sourceHandles.end() && sourceHandle->second == handle)
		{
			m_sourceHandles.erase(sourceHandle);
		}

		m_handles.erase(handle.Value);
		RetireActiveMesh(mesh->second);
		mesh = m_cache.erase(mesh);
	}
}

const GpuMesh* GpuMeshCache::Resolve(GpuMeshHandle handle) const noexcept
{
	if (!handle)
	{
		return nullptr;
	}

	const auto mesh = m_handles.find(handle.Value);
	return mesh != m_handles.end() ? mesh->second : nullptr;
}

std::size_t GpuMeshCache::GetCachedCount() const noexcept
{
	return m_cache.size();
}

bool GpuMeshCache::Contains(const Mesh& cpuMesh) const noexcept
{
	return m_sourceHandles.contains(&cpuMesh);
}

const GpuMesh* GpuMeshCache::Find(const Mesh& cpuMesh) const noexcept
{
	const auto source = m_sourceHandles.find(&cpuMesh);
	return source != m_sourceHandles.end() ? Resolve(source->second) : nullptr;
}

GpuMeshHandle GpuMeshCache::AllocateHandle()
{
	const GpuMeshHandle handle{m_nextGpuMeshHandle++};
	if (m_nextGpuMeshHandle == 0u)
	{
		Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh handle space is exhausted.");
	}

	return handle;
}

GpuMeshCache::MeshRequest* GpuMeshCache::FindRequest(const CacheKey& key) noexcept
{
	const auto request = m_requests.find(key);
	return request != m_requests.end() ? &request->second : nullptr;
}

void GpuMeshCache::LaunchPendingPreparations()
{
	const std::size_t activePreparationCount = static_cast<std::size_t>(std::count_if(
	    m_requests.begin(),
	    m_requests.end(),
	    [](const auto& entry) noexcept
	    {
		    const MeshRequest& request = entry.second;
		    return request.PreparationStarted && request.Execution.IsValid();
	    }));
	std::size_t availableSlots =
	    activePreparationCount < kMaximumConcurrentPreparations ? kMaximumConcurrentPreparations - activePreparationCount : 0;
	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		if (availableSlots == 0)
		{
			return;
		}
		if (request.PreparationStarted || !request.Wanted || request.Prepared == nullptr)
		{
			continue;
		}

		LaunchPreparation(request);
		--availableSlots;
	}
}

void GpuMeshCache::LaunchPreparation(MeshRequest& request)
{
	const ImmutableRenderMeshHandle mesh = request.Source;
	const std::shared_ptr<GpuMeshPreparedData> prepared = request.Prepared;
	request.Execution = m_taskExecutor->Launch(
	    *m_taskScope,
	    TaskDesc{.Name = TaskName("Prepare immutable mesh generation"), .Lane = TaskLane::Background},
	    [mesh, prepared](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    return TaskResult::Cancelled("Mesh preparation cancelled.");
		    }

		    *prepared = GpuMeshPreparation::Build(mesh);
		    return TaskResult::Success();
	    });
	request.PreparationStarted = true;
	if (!request.Execution.IsValid())
	{
		Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh preparation task launch failed.");
	}
}

void GpuMeshCache::ConsumeCompletedPreparations() noexcept
{
	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		if (!request.Execution.IsValid() || !request.Execution.IsSettled())
		{
			continue;
		}

		const TaskExecutionStatus status = request.Execution.GetStatus();
		const TaskResult result = request.Execution.GetResult();
		request.Execution = {};

		if (!request.Wanted)
		{
			(void) m_residency.Cancel(request.Generation);
			request.Prepared.reset();
			continue;
		}

		if (status != TaskExecutionStatus::Succeeded)
		{
			Diagnostics::Fatal(
			    g_gpuMeshCacheLogger,
			    __FILE__,
			    __LINE__,
			    result.GetMessage().empty() ? "GPU mesh preparation task failed." : result.GetMessage());
		}
		if (request.Prepared == nullptr)
		{
			Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh preparation produced no payload.");
		}

		if (!m_residency.BeginDecoding(request.Generation))
		{
			Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh generation could not enter decoding.");
		}
		const std::uint64_t decodedBytes = request.Prepared->GetDecodedByteSize();
		if (!m_residency.PublishReadyForUpload(request.Generation, decodedBytes, request.Prepared->GetResidentByteSize()))
		{
			Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh generation exceeded residency capacity.");
		}
	}
}

void GpuMeshCache::ActivateResidentMeshes() noexcept
{
	for (auto& entry : m_requests)
	{
		MeshRequest& request = entry.second;
		if (request.Uploaded == nullptr)
		{
			continue;
		}

		const AssetResidencyState state = m_residency.GetState(request.Generation);
		if (!request.Wanted && state == AssetResidencyState::Retired)
		{
			request.Uploaded.reset();
			continue;
		}

		if (!request.Wanted || state != AssetResidencyState::Resident)
		{
			continue;
		}

		const Mesh* const source = request.Source.GetResource().get();
		const auto [active, inserted] = m_cache.emplace(
		    request.Key,
		    ActiveMesh{.Generation = request.Generation, .Source = source, .Mesh = std::move(request.Uploaded)});
		if (!inserted)
		{
			Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "GPU mesh generation was activated twice.");
		}

		m_handles[request.Handle.Value] = active->second.Mesh.get();
		m_sourceHandles[source] = request.Handle;
	}
}

void GpuMeshCache::RemoveTerminalRequests() noexcept
{
	for (auto request = m_requests.begin(); request != m_requests.end();)
	{
		const AssetResidencyState state = m_residency.GetState(request->second.Generation);
		const bool terminal = !request->second.Execution.IsValid() && request->second.Prepared == nullptr
		    && request->second.Uploaded == nullptr && (state == AssetResidencyState::Resident || state == AssetResidencyState::Retired);
		if (terminal)
		{
			request = m_requests.erase(request);
		}
		else
		{
			++request;
		}
	}
}

void GpuMeshCache::RetireActiveMesh(ActiveMesh& mesh) noexcept
{
	if (!m_residency.BeginEviction(mesh.Generation, CaptureLastSubmittedState()))
	{
		Diagnostics::Fatal(g_gpuMeshCacheLogger, __FILE__, __LINE__, "Resident GPU mesh could not enter eviction.");
	}
	mesh.Mesh.reset();
}

RhiSubmissionState GpuMeshCache::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		state.MarkUsed(m_submissions->GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}

	return state;
}
