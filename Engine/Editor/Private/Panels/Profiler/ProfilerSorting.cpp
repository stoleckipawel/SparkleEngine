#include "PCH.h"
#include "Panels/Profiler/ProfilerSorting.h"

#include "Panels/Profiler/ProfilerSnapshotUtils.h"

#include <algorithm>

namespace ProfilerSorting
{
	static constexpr const char* kSortModeLabels[] = {
	    "Hierarchy (grouped)",
	    "Name (A-Z)",
	    "Name (Z-A)",
	    "Incl (high-low)",
	    "Excl (high-low)",
	    "Max (high-low)",
	    "Calls (high-low)",
	};

	const char* const* SortModeLabels() noexcept
	{
		return kSortModeLabels;
	}

	int SortModeCount() noexcept
	{
		return static_cast<int>(sizeof(kSortModeLabels) / sizeof(kSortModeLabels[0]));
	}

	void SortBucket(
	    std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
	    SortMode mode) noexcept
	{
		if (mode == SortMode::Hierarchy)
		{
			return;
		}
		std::stable_sort(
		    bucket.begin(),
		    bucket.end(),
		    [mode](const Diagnostics::ProfilerSnapshotNode* lhs,
		           const Diagnostics::ProfilerSnapshotNode* rhs)
		    {
			    switch (mode)
			    {
				    case SortMode::AlphabeticalAsc:
					    return lhs->Name < rhs->Name;
				    case SortMode::AlphabeticalDesc:
					    return lhs->Name > rhs->Name;
				    case SortMode::InclusiveDescending:
					    return lhs->AverageDurationMicroseconds > rhs->AverageDurationMicroseconds;
				    case SortMode::ExclusiveDescending:
					    return ProfilerSnapshotUtils::ComputeExclusiveMicroseconds(*lhs)
					           > ProfilerSnapshotUtils::ComputeExclusiveMicroseconds(*rhs);
				    case SortMode::MaxDescending:
					    return lhs->MaxDurationMicroseconds > rhs->MaxDurationMicroseconds;
				    case SortMode::CallsDescending:
					    return lhs->TotalCallCount > rhs->TotalCallCount;
				    case SortMode::Hierarchy:
				    default:
					    return false;
			    }
		    });
	}
}
