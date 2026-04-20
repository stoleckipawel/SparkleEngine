#include "PCH.h"
#include "Panels/Profiler/ProfilerSnapshotUtils.h"

#include <algorithm>
#include <cstdio>

namespace ProfilerSnapshotUtils
{
	// A child sum that exceeds the parent inclusive average by less than this
	// tolerance is treated as floating-point noise and not flagged.
	static constexpr double kExclusiveClampToleranceMicroseconds = 1.0;

	double SumChildAverageMicroseconds(const std::vector<ProfilerSnapshotNode>& children) noexcept
	{
		double sum = 0.0;
		for (const ProfilerSnapshotNode& child : children)
		{
			sum += child.AverageDurationMicroseconds;
		}
		return sum;
	}

	double ComputeExclusiveMicroseconds(const ProfilerSnapshotNode& node) noexcept
	{
		const double childSum = SumChildAverageMicroseconds(node.Children);
		return std::max(0.0, node.AverageDurationMicroseconds - childSum);
	}

	ExclusiveResult ComputeExclusiveWithStatus(const ProfilerSnapshotNode& node) noexcept
	{
		ExclusiveResult result;
		result.InclusiveMicroseconds = node.AverageDurationMicroseconds;
		result.ChildSumMicroseconds = SumChildAverageMicroseconds(node.Children);
		const double raw = result.InclusiveMicroseconds - result.ChildSumMicroseconds;
		result.ExclusiveMicroseconds = std::max(0.0, raw);
		result.WasClampedToZero = raw < -kExclusiveClampToleranceMicroseconds;
		return result;
	}

	const ProfilerSnapshotNode* FindNodeByName(
	    const std::vector<ProfilerSnapshotNode>& nodes,
	    std::string_view name) noexcept
	{
		for (const ProfilerSnapshotNode& node : nodes)
		{
			if (node.Name == name)
			{
				return &node;
			}
			if (const ProfilerSnapshotNode* found = FindNodeByName(node.Children, name))
			{
				return found;
			}
		}
		return nullptr;
	}

	const ProfilerSnapshotNode* FindNodeInBucket(
	    const std::vector<const ProfilerSnapshotNode*>& bucket,
	    std::string_view name) noexcept
	{
		for (const ProfilerSnapshotNode* node : bucket)
		{
			if (node->Name == name)
			{
				return node;
			}
			if (const ProfilerSnapshotNode* found = FindNodeByName(node->Children, name))
			{
				return found;
			}
		}
		return nullptr;
	}

	std::string_view ShortenScopeName(std::string_view fullName) noexcept
	{
		if (const std::size_t bracket = fullName.find("] "); bracket != std::string_view::npos)
		{
			return fullName.substr(bracket + 2);
		}
		if (const std::size_t dot = fullName.rfind('.'); dot != std::string_view::npos)
		{
			return fullName.substr(dot + 1);
		}
		return fullName;
	}

	std::string_view ExtractModuleName(std::string_view scopeName) noexcept
	{
		// CPU scopes use dotted names like `Renderer.RecordFrame`. Module is the first segment.
		if (const std::size_t dot = scopeName.find('.'); dot != std::string_view::npos)
		{
			return scopeName.substr(0, dot);
		}
		// GPU scopes use `[Kind #N] PassName`. Module is the kind inside brackets.
		if (!scopeName.empty() && scopeName.front() == '[')
		{
			if (const std::size_t space = scopeName.find(' '); space != std::string_view::npos && space > 1)
			{
				return scopeName.substr(1, space - 1);
			}
		}
		return scopeName;
	}

	void FormatThreadLabel(
	    char* buffer,
	    std::size_t bufferSize,
	    const Engine::Diagnostics::ProfilerThreadSnapshot& thread) noexcept
	{
		if (!thread.ThreadName.empty())
		{
			std::snprintf(buffer, bufferSize, "%s (TID %u)", thread.ThreadName.c_str(), thread.ThreadId);
		}
		else
		{
			std::snprintf(buffer, bufferSize, "Thread %u", thread.ThreadId);
		}
	}
}
