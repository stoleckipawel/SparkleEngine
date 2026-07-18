#pragma once

#include "TaskTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace TaskDetail
{
	struct CompiledTaskGraphData;
	struct TaskGraphAccess;
}

class TaskExecution;
class TaskExecutor;

class SPARKLE_TASKS_API TaskNodeHandle final
{
  public:
	TaskNodeHandle() noexcept = default;

	bool IsValid() const noexcept { return m_indexPlusOne != 0; }
	explicit operator bool() const noexcept { return IsValid(); }
	bool operator==(const TaskNodeHandle&) const noexcept = default;

  private:
	friend class TaskGraphBuilder;
	friend class TaskExecution;
	friend class TaskExecutor;
	friend struct TaskDetail::TaskGraphAccess;

	TaskNodeHandle(std::uint64_t builderIdentity, std::uint32_t builderGeneration, std::uint32_t index) noexcept :
	    m_builderIdentity(builderIdentity), m_builderGeneration(builderGeneration), m_indexPlusOne(index + 1u)
	{
	}

	std::uint64_t m_builderIdentity = 0;
	std::uint32_t m_builderGeneration = 0;
	std::uint32_t m_indexPlusOne = 0;
};

struct TaskGraphLimits final
{
	static constexpr std::uint32_t HardMaximumTasks = 65'535;
	static constexpr std::uint32_t HardMaximumEdges = 262'144;

	std::uint32_t MaximumTasks = 1'024;
	std::uint32_t MaximumEdges = 4'096;
};

enum class TaskGraphErrorCode : std::uint8_t
{
	None,
	InvalidLimits,
	InvalidTaskName,
	InvalidCompletionPolicy,
	TaskCapacityExceeded,
	EdgeCapacityExceeded,
	InvalidHandle,
	ForeignHandle,
	StaleHandle,
	GenerationExhausted,
	SelfDependency,
	DuplicateDependency,
	Cycle
};

struct TaskGraphError final
{
	TaskGraphErrorCode Code = TaskGraphErrorCode::None;
	std::string Message;

	explicit operator bool() const noexcept { return Code != TaskGraphErrorCode::None; }
};

class SPARKLE_TASKS_API CompiledTaskGraph final
{
  public:
	CompiledTaskGraph() noexcept = default;

	bool IsValid() const noexcept;
	explicit operator bool() const noexcept { return IsValid(); }

	const TaskGraphError& GetError() const noexcept;
	std::uint32_t GetTaskCount() const noexcept;
	std::uint32_t GetEdgeCount() const noexcept;

  private:
	friend class TaskGraphBuilder;
	friend class TaskExecutor;

	explicit CompiledTaskGraph(std::shared_ptr<const TaskDetail::CompiledTaskGraphData> data) noexcept;

	std::shared_ptr<const TaskDetail::CompiledTaskGraphData> m_data;
};

class SPARKLE_TASKS_API TaskGraphBuilder final
{
  public:
	explicit TaskGraphBuilder(TaskGraphLimits limits = {});
	~TaskGraphBuilder();

	TaskGraphBuilder(const TaskGraphBuilder&) = delete;
	TaskGraphBuilder& operator=(const TaskGraphBuilder&) = delete;
	TaskGraphBuilder(TaskGraphBuilder&&) = delete;
	TaskGraphBuilder& operator=(TaskGraphBuilder&&) = delete;

	TaskNodeHandle Add(TaskDesc desc, TaskFunction function);
	TaskNodeHandle AddNested(TaskNodeHandle parent, TaskDesc desc, TaskFunction function);
	bool DependsOn(TaskNodeHandle task, TaskNodeHandle prerequisite);
	TaskNodeHandle WhenAll(TaskName name, std::span<const TaskNodeHandle> prerequisites);
	TaskNodeHandle ContinueWith(TaskNodeHandle prerequisite, TaskDesc desc, TaskFunction function);

	CompiledTaskGraph Compile() const;
	void Reset() noexcept;

	const TaskGraphError& GetError() const noexcept;

  private:
	struct State;
	std::unique_ptr<State> m_state;
};
