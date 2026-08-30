#include "PCH.h"

#include "Level/Loading/SceneLoadExecutor.h"

#include "Assets/SceneAssetCatalog.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Level/Loading/SceneLoadWorkState.h"
#include "Level/Loading/SceneLoadTaskGraph.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"

#include <chrono>
#include <format>
#include <unordered_set>

class SceneLoadLimits final
{
public:
	static constexpr std::size_t kMaximumAssetsPerRequest = 256;
};

static const auto g_sceneLoadExecutorLogger = Logging::GetOrCreateLogger("GameFramework.SceneLoadExecutor");

namespace Assets
{
	struct SceneLoadExecutor::ControlState final
	{
		ControlState(TaskExecutor& executor, TaskScope& applicationScope) :
		    Executor(executor),
		    ApplicationScope(applicationScope)
		{
		}

		TaskExecutor& Executor;
		TaskScope& ApplicationScope;
		std::unique_ptr<TaskScope> Scope;
		TaskExecution Execution;
		std::shared_ptr<SceneLoadWorkState> Shared;
		std::shared_ptr<const SceneAssetCatalog> Catalog;
		std::uint64_t NextCatalogGeneration = 1;
		std::uint64_t RequestId = 0;
	};

	SceneLoadExecutor::SceneLoadExecutor(TaskExecutor& executor, TaskScope& applicationScope) :
	    m_control(std::make_unique<ControlState>(executor, applicationScope))
	{
	}

	SceneLoadExecutor::~SceneLoadExecutor()
	{
		Cancel();
		if (m_control->Scope)
			m_control->Scope->JoinFor(std::chrono::milliseconds::max());
	}

	void SceneLoadExecutor::Start(std::uint64_t requestId, std::uint64_t worldGeneration, std::uint64_t documentGeneration, LevelDesc level)
	{
		if (m_control->Scope)
		{
			Diagnostics::Fatal(
			    g_sceneLoadExecutorLogger,
			    __FILE__,
			    __LINE__,
			    "A scene-load task graph was started before the previous graph settled.");
		}
		if (level.sceneAssetIds.size() > SceneLoadLimits::kMaximumAssetsPerRequest)
		{
			throw Diagnostics::Error("Level exceeds the bounded scene-asset request capacity.");
		}

		if (!m_control->Catalog)
		{
			m_control->Catalog = LoadSceneAssetCatalog(m_control->NextCatalogGeneration++);
		}

		auto shared = std::make_shared<SceneLoadWorkState>();
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
				throw Diagnostics::Error(
				    id.IsEmpty() ? "Level contains an empty scene asset identity."
				                 : std::format("Level contains duplicate scene asset identity '{}'.", id.value));
			}
			const std::optional<std::filesystem::path> manifest = m_control->Catalog->Resolve(id.GetCatalogValue());
			if (!manifest)
			{
				throw Diagnostics::Error(
				    std::format(
				        "Scene asset catalog ID '{}' for instance '{}' is absent from catalog generation {}.",
				        id.GetCatalogValue(),
				        id.value,
				        m_control->Catalog->GetGeneration()));
			}
			shared->Assets.push_back(SceneAssetLoadWork{.Id = id, .ManifestPath = *manifest});
		}

		const CompiledTaskGraph graph = BuildSceneLoadTaskGraph(shared);
		if (!graph)
		{
			Diagnostics::Fatal(g_sceneLoadExecutorLogger, __FILE__, __LINE__, graph.GetError().Message);
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
			Diagnostics::Fatal(g_sceneLoadExecutorLogger, __FILE__, __LINE__, "SparkleTasks rejected the scene-load task graph.");
		}
	}

	void SceneLoadExecutor::Cancel() noexcept
	{
		if (m_control->Scope)
			m_control->Scope->Cancel();
	}

	std::optional<SceneLoadCompletion> SceneLoadExecutor::ConsumeSettled()
	{
		if (!m_control->Execution.IsValid() || !m_control->Execution.IsSettled())
		{
			return std::nullopt;
		}

		SceneLoadCompletion completion;
		completion.RequestId = m_control->RequestId;
		const TaskResult result = m_control->Execution.GetResult();
		completion.Stage = result.Succeeded()
		    ? LevelLoadOperationStage::Ready
		    : (result.WasCancelled() ? LevelLoadOperationStage::Cancelled : LevelLoadOperationStage::Failed);
		completion.Diagnostic = std::string(result.GetMessage());
		if (result.Succeeded())
		{
			if (!m_control->Shared || !m_control->Shared->Package)
			{
				Diagnostics::Fatal(
				    g_sceneLoadExecutorLogger,
				    __FILE__,
				    __LINE__,
				    "A successful scene-load task graph produced no package.");
			}
			completion.Package = std::move(m_control->Shared->Package);
		}

		if (!m_control->Scope)
		{
			Diagnostics::Fatal(g_sceneLoadExecutorLogger, __FILE__, __LINE__, "Settled scene-load execution has no owning task scope.");
		}
		m_control->Scope->JoinFor(std::chrono::milliseconds::zero());
		m_control->Execution = {};
		m_control->Scope.reset();
		m_control->Shared.reset();
		return completion;
	}

	LevelLoadOperationProgress SceneLoadExecutor::GetProgress() const noexcept
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

	std::uint64_t SceneLoadExecutor::GetCatalogGeneration() const noexcept
	{
		return m_control->Catalog ? m_control->Catalog->GetGeneration() : 0;
	}
}
