#pragma once

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <vector>

namespace ProfilerSorting
{
	enum class SortMode : int
	{
		Hierarchy = 0,
		AlphabeticalAsc,
		AlphabeticalDesc,
		InclusiveDescending,
		ExclusiveDescending,
		MaxDescending,
		CallsDescending,
	};

	// UI labels in the same order as the enum, suitable for ImGui::Combo().
	const char* const* SortModeLabels() noexcept;
	int SortModeCount() noexcept;

	// Stable-sorts a flat bucket of node pointers in place. `Hierarchy` is a
	// no-op (preserves capture order so the table mirrors the call tree).
	void SortBucket(
	    std::vector<const Diagnostics::ProfilerSnapshotNode*>& bucket,
	    SortMode mode) noexcept;
}
