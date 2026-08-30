#pragma once

#include "Level/Loading/SceneLoadPackage.h"
#include "Level/LevelLoadOperation.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
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
	};

	class SceneLoadExecutor final
	{
	public:
		SceneLoadExecutor(TaskExecutor& executor, TaskScope& applicationScope);
		~SceneLoadExecutor();

		SceneLoadExecutor(const SceneLoadExecutor&) = delete;
		SceneLoadExecutor& operator=(const SceneLoadExecutor&) = delete;

		void Start(std::uint64_t requestId, std::uint64_t worldGeneration, std::uint64_t documentGeneration, LevelDesc level);
		void Cancel() noexcept;
		std::optional<SceneLoadCompletion> ConsumeSettled();
		LevelLoadOperationProgress GetProgress() const noexcept;
		std::uint64_t GetCatalogGeneration() const noexcept;

	private:
		struct ControlState;
		std::unique_ptr<ControlState> m_control;
	};
}
