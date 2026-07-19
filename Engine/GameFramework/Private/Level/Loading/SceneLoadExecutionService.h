#pragma once

#include "Level/Loading/SceneLoadPackage.h"
#include "Level/LevelLoadOperation.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class TaskExecutor;
class TaskScope;

namespace Assets
{
	struct SceneLoadCompletion final
	{
		std::unique_ptr<SceneLoadPackage> Package;
		std::uint64_t RequestId = 0;
		LevelLoadOperationStage Stage = LevelLoadOperationStage::Idle;
		std::string Diagnostic;

		bool Succeeded() const noexcept { return Stage == LevelLoadOperationStage::Ready && Package != nullptr; }
	};

	class SceneLoadExecutionService final
	{
	  public:
		SceneLoadExecutionService(TaskExecutor& executor, TaskScope& applicationScope);
		~SceneLoadExecutionService();

		SceneLoadExecutionService(const SceneLoadExecutionService&) = delete;
		SceneLoadExecutionService& operator=(const SceneLoadExecutionService&) = delete;

		bool Start(
		    std::uint64_t requestId,
		    std::uint64_t worldGeneration,
		    std::uint64_t documentGeneration,
		    LevelDesc level,
		    std::string& errorMessage);
		void Cancel() noexcept;
		bool TryConsume(SceneLoadCompletion& completion) noexcept;
		LevelLoadOperationProgress GetProgress() const noexcept;
		std::uint64_t GetCatalogGeneration() const noexcept;

	  private:
		struct ControlState;
		std::unique_ptr<ControlState> m_control;
	};
}
