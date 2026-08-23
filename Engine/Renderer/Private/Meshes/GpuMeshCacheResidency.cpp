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
#include <utility>

static const auto g_gpuMeshCacheLogger = Logging::GetOrCreateLogger("Renderer.GpuMeshCache");

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
