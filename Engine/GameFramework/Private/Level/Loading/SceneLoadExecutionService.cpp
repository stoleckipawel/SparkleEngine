#include "PCH.h"

#include "Level/Loading/SceneLoadExecutionService.h"

#include "Assets/SceneAssetCatalog.h"
#include "Level/Loading/SceneLoadExecutionState.h"
#include "Level/Loading/SceneLoadTaskGraph.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <chrono>
#include <format>
#include <unordered_set>

namespace
{
	constexpr std::size_t kMaximumAssetsPerRequest = 256;
}

namespace Assets
{
	struct SceneLoadExecutionService::ControlState final
	{
		ControlState(TaskExecutor& executor, TaskScope& applicationScope) :
		    Executor(executor), ApplicationScope(applicationScope)
		{
		}

		TaskExecutor& Executor;
		TaskScope& ApplicationScope;
		std::unique_ptr<TaskScope> Scope;
		TaskExecution Execution;
		std::shared_ptr<SceneLoadSharedState> Shared;
		std::shared_ptr<const SceneAssetCatalog> Catalog;
		std::uint64_t NextCatalogGeneration = 1;
		std::uint64_t RequestId = 0;
	};

	SceneLoadExecutionService::SceneLoadExecutionService(
	    TaskExecutor& executor,
	    TaskScope& applicationScope) :
	    m_control(std::make_unique<ControlState>(executor, applicationScope))
	{
	}

	SceneLoadExecutionService::~SceneLoadExecutionService()
	{
		Cancel();
		if (m_control->Scope)
			m_control->Scope->JoinFor(std::chrono::milliseconds::max());
	}

	bool SceneLoadExecutionService::Start(
	    std::uint64_t requestId,
	    std::uint64_t worldGeneration,
	    std::uint64_t documentGeneration,
	    LevelDesc level,
	    std::string& errorMessage)
	{
		if (m_control->Scope)
		{
			errorMessage = "The previous scene load execution has not settled.";
			return false;
		}
		if (level.sceneAssetIds.size() > kMaximumAssetsPerRequest)
		{
			errorMessage = "Level exceeds the bounded scene-asset request capacity.";
			return false;
		}

		if (!m_control->Catalog)
			m_control->Catalog = LoadSceneAssetCatalog(m_control->NextCatalogGeneration++, errorMessage);
		if (!m_control->Catalog)
			return false;

		auto shared = std::make_shared<SceneLoadSharedState>();
		shared->Package = std::make_unique<SceneLoadPackage>();
		shared->Package->RequestId = requestId;
		shared->Package->WorldGeneration = worldGeneration;
		shared->Package->DocumentGeneration = documentGeneration;
		shared->Package->CatalogGeneration = m_control->Catalog->GetGeneration();
		shared->Package->Level = std::move(level);
		shared->Assets.reserve(shared->Package->Level.sceneAssetIds.size());

		std::unordered_set<std::string> identities;
		for (const SceneAssetId& id : shared->Package->Level.sceneAssetIds)
		{
			if (id.IsEmpty() || !identities.insert(id.value).second)
			{
				errorMessage = id.IsEmpty() ? "Level contains an empty scene asset identity."
				                            : std::format("Level contains duplicate scene asset identity '{}'.", id.value);
				return false;
			}
			const std::optional<std::filesystem::path> manifest = m_control->Catalog->Resolve(id.value);
			if (!manifest)
			{
				errorMessage = std::format(
				    "Scene asset '{}' is absent from catalog generation {}.", id.value, m_control->Catalog->GetGeneration());
				return false;
			}
			shared->Assets.push_back(SceneAssetLoadWork{.Id = id, .ManifestPath = *manifest});
		}

		const CompiledTaskGraph graph = BuildSceneLoadTaskGraph(shared);
		if (!graph)
		{
			errorMessage = graph.GetError().Message;
			return false;
		}

		m_control->Scope = std::make_unique<TaskScope>(
		    TaskScopeDesc{TaskScopeKind::Document, std::format("Load level {}", shared->Package->Level.name)},
		    &m_control->ApplicationScope);
		m_control->RequestId = requestId;
		m_control->Shared = shared;
		m_control->Execution = m_control->Executor.Launch(*m_control->Scope, graph, TaskExecutionContext(shared));
		if (!m_control->Execution.IsValid())
		{
			m_control->Scope->Cancel();
			m_control->Scope->JoinFor(std::chrono::milliseconds::zero());
			m_control->Scope.reset();
			m_control->Shared.reset();
			errorMessage = "SparkleTasks rejected the scene load execution.";
			return false;
		}
		errorMessage.clear();
		return true;
	}

	void SceneLoadExecutionService::Cancel() noexcept
	{
		if (m_control->Scope)
			m_control->Scope->Cancel();
	}

	bool SceneLoadExecutionService::TryConsume(SceneLoadCompletion& completion) noexcept
	{
		if (!m_control->Execution.IsValid() || !m_control->Execution.IsSettled())
			return false;

		completion = {};
		completion.RequestId = m_control->RequestId;
		const TaskResult result = m_control->Execution.GetResult();
		completion.Stage = result.Succeeded()
		                       ? LevelLoadOperationStage::Ready
		                       : (result.WasCancelled() ? LevelLoadOperationStage::Cancelled : LevelLoadOperationStage::Failed);
		completion.Diagnostic = std::string(result.GetMessage());
		if (result.Succeeded() && m_control->Shared)
			completion.Package = std::move(m_control->Shared->Package);

		m_control->Scope->JoinFor(std::chrono::milliseconds::zero());
		m_control->Execution = {};
		m_control->Scope.reset();
		m_control->Shared.reset();
		return true;
	}

	LevelLoadOperationProgress SceneLoadExecutionService::GetProgress() const noexcept
	{
		LevelLoadOperationProgress progress;
		progress.RequestId = m_control->RequestId;
		if (!m_control->Shared)
			return progress;
		progress.Stage = m_control->Shared->Stage.load(std::memory_order_acquire);
		progress.CompletedAssets = m_control->Shared->CompletedDecodes.load(std::memory_order_relaxed);
		progress.TotalAssets = static_cast<std::uint32_t>(m_control->Shared->Assets.size());
		return progress;
	}

	std::uint64_t SceneLoadExecutionService::GetCatalogGeneration() const noexcept
	{
		return m_control->Catalog ? m_control->Catalog->GetGeneration() : 0;
	}
}
