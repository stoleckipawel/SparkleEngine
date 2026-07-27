#include "PCH.h"
#include "Meshes/GPUMeshCache.h"

#include "Meshes/GPUMeshPreparation.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <utility>

GPUMeshCache::GPUMeshCache(
    RenderHardwareInterface& renderHardwareInterface,
    RhiCommandSubmissionService& submissions,
    TaskExecutor& taskExecutor,
    TaskScope& applicationScope) :
	m_renderHardwareInterface(&renderHardwareInterface),
	m_submissions(&submissions),
	m_taskExecutor(&taskExecutor),
	m_taskScope(std::make_unique<TaskScope>(
	    TaskScopeDesc{
	        TaskScopeKind::AssetGeneration,
	        "Renderer mesh generations"},
	    &applicationScope))
{
}

GPUMeshCache::~GPUMeshCache() noexcept
{
	if (m_taskScope != nullptr)
	{
		m_taskScope->Cancel();
		(void)m_taskScope->JoinFor(
		    std::chrono::milliseconds::max());
	}

	m_requests.clear();
	m_sourceHandles.clear();
	m_handles.clear();
	m_cache.clear();
}

GpuMeshHandle GPUMeshCache::Request(
    const ImmutableRenderMeshHandle& mesh)
{
	if (!mesh.IsValid())
	{
		return {};
	}

	const CacheKey key{
	    mesh.GetAssetId(),
	    mesh.GetGeneration()};
	const auto active = m_cache.find(key);
	if (active != m_cache.end())
	{
		return active->second.Mesh->GetHandle();
	}

	MeshRequest* const existingRequest = FindRequest(key);
	if (existingRequest != nullptr)
	{
		existingRequest->Wanted = true;
		return existingRequest->Handle;
	}

	const std::optional<AssetGenerationHandle> generation =
	    m_residency.BeginGeneration(key.first, key.second);
	if (!generation || m_taskScope == nullptr)
	{
		return {};
	}

	const GpuMeshHandle handle = AllocateHandle();
	auto prepared = std::make_shared<GPUMeshPreparedData>();

	MeshRequest request;
	request.Key = key;
	request.Source = mesh;
	request.Handle = handle;
	request.Generation = *generation;
	request.Prepared = prepared;
	request.Execution = m_taskExecutor->Launch(
	    *m_taskScope,
	    TaskDesc{
	        .Name = TaskName("Prepare immutable mesh generation"),
	        .Lane = TaskLane::Background},
	    [mesh, prepared](TaskExecutionContext& context)
	    {
		    if (context.IsCancellationRequested())
		    {
			    return TaskResult::Cancelled(
			        "Mesh preparation cancelled.");
		    }

		    return GPUMeshPreparation::Build(mesh, *prepared)
		               ? TaskResult::Success()
		               : TaskResult::Failure(
		                     "Mesh preparation failed.");
	    });

	if (!request.Execution.IsValid())
	{
		(void)m_residency.MarkFailed(*generation);
		m_failedHandles.insert(handle.Value);
		return handle;
	}

	m_requests.push_back(std::move(request));
	return handle;
}

void GPUMeshCache::UploadReadyMeshes()
{
	ConsumeCompletedPreparations();

	for (MeshRequest& request : m_requests)
	{
		if (!request.Wanted ||
		    request.Prepared == nullptr ||
		    request.Uploaded != nullptr ||
		    m_residency.GetState(request.Generation) !=
		        AssetResidencyState::ReadyForUpload ||
		    !m_residency.BeginUpload(request.Generation))
		{
			continue;
		}

		request.ResidentBytes =
		    request.Prepared->GetResidentByteSize();
		auto gpuMesh =
		    std::make_unique<GPUMesh>(request.Handle);
		if (!gpuMesh->Upload(
		        *m_renderHardwareInterface,
		        std::move(*request.Prepared)))
		{
			(void)m_residency.MarkFailed(
			    request.Generation);
			m_failedHandles.insert(request.Handle.Value);
			request.Prepared.reset();
			continue;
		}

		request.Prepared.reset();
		request.Uploaded = std::move(gpuMesh);
	}
}

void GPUMeshCache::RecordUploadSubmission(
    RhiSubmissionToken token) noexcept
{
	if (!token.IsValid())
	{
		return;
	}

	for (MeshRequest& request : m_requests)
	{
		if (request.Uploaded != nullptr &&
		    !request.UploadSubmitted)
		{
			request.UploadSubmitted =
			    m_residency.RecordUploadSubmission(
			        request.Generation,
			        token,
			        request.ResidentBytes);
		}
	}
}

void GPUMeshCache::PollResidency() noexcept
{
	ConsumeCompletedPreparations();
	m_residency.Poll(*m_submissions);
	ActivateResidentMeshes();
	RemoveTerminalRequests();
}

void GPUMeshCache::RetainOnly(
    std::span<const GpuMeshHandle> handles) noexcept
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

	std::sort(
	    wantedHandles.begin(),
	    wantedHandles.end());
	wantedHandles.erase(
	    std::unique(
	        wantedHandles.begin(),
	        wantedHandles.end()),
	    wantedHandles.end());

	const auto isWanted =
	    [&wantedHandles](GpuMeshHandle handle) noexcept
	    {
		    return std::binary_search(
		        wantedHandles.begin(),
		        wantedHandles.end(),
		        handle.Value);
	    };

	for (MeshRequest& request : m_requests)
	{
		request.Wanted = isWanted(request.Handle);
		if (!request.Wanted)
		{
			(void)m_residency.Cancel(
			    request.Generation);
			if (!request.Execution.IsValid() &&
			    request.Uploaded == nullptr)
			{
				request.Prepared.reset();
			}
		}
	}

	for (auto mesh = m_cache.begin();
	     mesh != m_cache.end();)
	{
		const GpuMeshHandle handle =
		    mesh->second.Mesh->GetHandle();
		if (isWanted(handle))
		{
			++mesh;
			continue;
		}

		const auto sourceHandle =
		    m_sourceHandles.find(mesh->second.Source);
		if (sourceHandle != m_sourceHandles.end() &&
		    sourceHandle->second == handle)
		{
			m_sourceHandles.erase(sourceHandle);
		}

		m_handles.erase(handle.Value);
		RetireActiveMesh(mesh->second);
		mesh = m_cache.erase(mesh);
	}

	for (auto failed = m_failedHandles.begin();
	     failed != m_failedHandles.end();)
	{
		if (!std::binary_search(
		        wantedHandles.begin(),
		        wantedHandles.end(),
		        *failed))
		{
			failed = m_failedHandles.erase(failed);
		}
		else
		{
			++failed;
		}
	}
}

const GPUMesh* GPUMeshCache::Resolve(
    GpuMeshHandle handle) const noexcept
{
	if (!handle)
	{
		return nullptr;
	}

	const auto mesh = m_handles.find(handle.Value);
	return mesh != m_handles.end()
	           ? mesh->second
	           : nullptr;
}

bool GPUMeshCache::HasFailed(
    GpuMeshHandle handle) const noexcept
{
	return handle &&
	       m_failedHandles.contains(handle.Value);
}

std::size_t GPUMeshCache::GetCachedCount() const noexcept
{
	return m_cache.size();
}

bool GPUMeshCache::Contains(
    const Mesh& cpuMesh) const noexcept
{
	return m_sourceHandles.contains(&cpuMesh);
}

const GPUMesh* GPUMeshCache::Find(
    const Mesh& cpuMesh) const noexcept
{
	const auto source = m_sourceHandles.find(&cpuMesh);
	return source != m_sourceHandles.end()
	           ? Resolve(source->second)
	           : nullptr;
}

GpuMeshHandle GPUMeshCache::AllocateHandle() noexcept
{
	const GpuMeshHandle handle{
	    m_nextGpuMeshHandle++};
	if (m_nextGpuMeshHandle == 0u)
	{
		m_nextGpuMeshHandle = 1u;
	}

	return handle;
}

GPUMeshCache::MeshRequest* GPUMeshCache::FindRequest(
    const CacheKey& key) noexcept
{
	const auto request = std::find_if(
	    m_requests.begin(),
	    m_requests.end(),
	    [&key](const MeshRequest& candidate) noexcept
	    {
		    return candidate.Key == key;
	    });
	return request != m_requests.end()
	           ? &*request
	           : nullptr;
}

void GPUMeshCache::ConsumeCompletedPreparations() noexcept
{
	for (MeshRequest& request : m_requests)
	{
		if (!request.Execution.IsValid() ||
		    !request.Execution.IsSettled())
		{
			continue;
		}

		const TaskExecutionStatus status =
		    request.Execution.GetStatus();
		request.Execution = {};

		if (!request.Wanted ||
		    status == TaskExecutionStatus::Cancelled)
		{
			(void)m_residency.Cancel(
			    request.Generation);
			request.Prepared.reset();
			continue;
		}

		if (status != TaskExecutionStatus::Succeeded ||
		    request.Prepared == nullptr ||
		    !request.Prepared->IsValid())
		{
			(void)m_residency.MarkFailed(
			    request.Generation);
			m_failedHandles.insert(
			    request.Handle.Value);
			request.Prepared.reset();
			continue;
		}

		(void)m_residency.BeginDecoding(
		    request.Generation);
		const std::uint64_t decodedBytes =
		    request.Prepared->GetDecodedByteSize();
		if (!m_residency.PublishReadyForUpload(
		        request.Generation,
		        decodedBytes,
		        request.Prepared->GetResidentByteSize()))
		{
			(void)m_residency.MarkFailed(
			    request.Generation);
			m_failedHandles.insert(
			    request.Handle.Value);
			request.Prepared.reset();
		}
	}
}

void GPUMeshCache::ActivateResidentMeshes() noexcept
{
	for (MeshRequest& request : m_requests)
	{
		if (request.Uploaded == nullptr)
		{
			continue;
		}

		const AssetResidencyState state =
		    m_residency.GetState(request.Generation);
		if (!request.Wanted &&
		    (state == AssetResidencyState::Retired ||
		     state == AssetResidencyState::Failed))
		{
			request.Uploaded.reset();
			continue;
		}

		if (!request.Wanted ||
		    state != AssetResidencyState::Resident)
		{
			continue;
		}

		const Mesh* const source =
		    request.Source.GetResource().get();
		const auto [active, inserted] = m_cache.emplace(
		    request.Key,
		    ActiveMesh{
		        .Generation = request.Generation,
		        .Source = source,
		        .Mesh = std::move(request.Uploaded)});
		if (!inserted)
		{
			m_failedHandles.insert(
			    request.Handle.Value);
			continue;
		}

		m_handles[request.Handle.Value] =
		    active->second.Mesh.get();
		m_sourceHandles[source] = request.Handle;
	}
}

void GPUMeshCache::RemoveTerminalRequests() noexcept
{
	m_requests.erase(
	    std::remove_if(
	        m_requests.begin(),
	        m_requests.end(),
	        [this](const MeshRequest& request) noexcept
	        {
		        const AssetResidencyState state =
		            m_residency.GetState(
		                request.Generation);
		        return !request.Execution.IsValid() &&
		               request.Prepared == nullptr &&
		               request.Uploaded == nullptr &&
		               (state == AssetResidencyState::Resident ||
		                state == AssetResidencyState::Retired ||
		                state == AssetResidencyState::Failed);
	        }),
	    m_requests.end());
}

void GPUMeshCache::RetireActiveMesh(
    ActiveMesh& mesh) noexcept
{
	(void)m_residency.BeginEviction(
	    mesh.Generation,
	    CaptureLastSubmittedState());
	mesh.Mesh.reset();
}

RhiSubmissionState
GPUMeshCache::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	for (std::size_t queueIndex = 0;
	     queueIndex < RhiQueueTypeCount;
	     ++queueIndex)
	{
		state.MarkUsed(
		    m_submissions->GetLastSubmittedToken(
		        static_cast<ERhiQueueType>(
		            queueIndex)));
	}

	return state;
}
