#pragma once

#include "TaskGraph.h"
#include "TaskGraphStorage.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct TaskGraphBuilder::State final
{
	class Compilation;

	explicit State(TaskGraphLimits requestedLimits);

	static std::uint64_t AcquireBuilderIdentity() noexcept;
	static bool AreValidLimits(TaskGraphLimits limits) noexcept;
	static bool IsValidCompletionPolicy(TaskCompletionPolicy policy) noexcept;
	static bool IsValidTaskLane(TaskLane lane) noexcept;
	static TaskGraphError CreateError(TaskGraphErrorCode code, std::string message);

	void RecordError(TaskGraphErrorCode code, std::string message);
	bool ResolveHandle(TaskNodeHandle handle, std::uint32_t& outIndex);
	std::shared_ptr<TaskGraphStorage> Compile() const;

	TaskGraphLimits Limits;
	std::uint64_t BuilderIdentity = 0;
	std::uint32_t BuilderGeneration = 1;
	std::uint32_t EdgeCount = 0;
	TaskGraphError Error;
	std::vector<TaskGraphNode> Nodes;
};
