#include "ParallelFor.h"

#include "TaskExecutionContext.h"
#include "TaskGraphInternal.h"

#include <algorithm>
#include <utility>

class ParallelForOperations final
{
  public:
	static TaskName DerivedTaskName(const TaskName& base, std::string suffix)
	{
		std::string value(base.Get());
		if (value.size() + suffix.size() > TaskName::MaximumLength)
		{
			value.resize(TaskName::MaximumLength - suffix.size());
		}
		value += suffix;
		return TaskName(value);
	}
};

TaskNodeHandle ParallelFor(
    TaskGraphBuilder& graph,
    TaskDesc desc,
    std::uint32_t itemCount,
    ParallelForPolicy policy,
    ParallelForFunction function)
{
	if (policy.GrainSize == 0 || policy.MaximumPartitions == 0)
	{
		TaskDetail::TaskGraphAccess::RecordError(
		    graph,
		    TaskGraphErrorCode::InvalidParallelForPolicy,
		    "ParallelFor grain size and maximum partitions must be non-zero.");
		return {};
	}

	if (itemCount <= policy.SerialThreshold || itemCount <= policy.GrainSize)
	{
		return graph.Add(
		    std::move(desc),
		    [itemCount, function = std::move(function)](TaskExecutionContext& context)
		    {
			return function ? function(0, itemCount, context) : TaskResult::Success();
		    });
	}

	TaskDesc groupDesc = desc;
	groupDesc.Name = ParallelForOperations::DerivedTaskName(desc.Name, ".Group");
	const TaskNodeHandle group = graph.Add(std::move(groupDesc), [](TaskExecutionContext&) { return TaskResult::Success(); });
	if (!group)
	{
		return {};
	}

	const auto divideRoundUp = [](std::uint32_t value, std::uint32_t divisor)
	{
		return static_cast<std::uint32_t>((static_cast<std::uint64_t>(value) + divisor - 1u) / divisor);
	};
	const std::uint32_t partitionCount = std::min(policy.MaximumPartitions, divideRoundUp(itemCount, policy.GrainSize));
	const std::uint32_t partitionSize = divideRoundUp(itemCount, partitionCount);
	for (std::uint32_t partition = 0; partition < partitionCount; ++partition)
	{
		const std::uint32_t begin = partition * partitionSize;
		const std::uint32_t end = std::min(itemCount, begin + partitionSize);
		if (begin == end)
		{
			break;
		}

		TaskDesc partitionDesc = desc;
		partitionDesc.Name = ParallelForOperations::DerivedTaskName(desc.Name, ".Range" + std::to_string(partition));
		graph.AddNested(
		    group,
		    std::move(partitionDesc),
		    [begin, end, function](TaskExecutionContext& context)
		    {
			return function ? function(begin, end, context) : TaskResult::Success();
		    });
	}
	return group;
}
