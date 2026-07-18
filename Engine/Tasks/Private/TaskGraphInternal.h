#pragma once

#include "TaskGraph.h"

#include <limits>
#include <optional>
#include <vector>

namespace TaskDetail
{
	inline constexpr std::uint32_t InvalidTaskIndex = std::numeric_limits<std::uint32_t>::max();

	struct CompiledTaskNode final
	{
		TaskDesc Desc;
		TaskFunction Function;
		std::vector<std::uint32_t> Prerequisites;
		std::vector<std::uint32_t> Dependents;
		std::optional<std::uint32_t> Parent;
		std::vector<std::uint32_t> NestedChildren;
	};

	struct CompiledTaskGraphData final
	{
		TaskGraphError Error;
		TaskGraphLimits Limits;
		std::uint64_t BuilderIdentity = 0;
		std::uint32_t BuilderGeneration = 0;
		std::uint32_t EdgeCount = 0;
		std::vector<CompiledTaskNode> Nodes;
	};

	struct TaskGraphAccess final
	{
		static std::uint64_t GetBuilderIdentity(TaskNodeHandle handle) noexcept;
		static std::uint32_t GetBuilderGeneration(TaskNodeHandle handle) noexcept;
		static std::uint32_t GetIndex(TaskNodeHandle handle) noexcept;
		static bool Decode(
		    TaskNodeHandle handle,
		    std::uint64_t builderIdentity,
		    std::uint32_t builderGeneration,
		    std::uint32_t taskCount,
		    std::uint32_t& outIndex) noexcept;
	};
}
