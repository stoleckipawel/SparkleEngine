#pragma once

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <string>
#include <string_view>
#include <vector>

namespace ProfilerSnapshotUtils
{
	using Diagnostics::ProfilerSnapshotNode;

	// ---- Time helpers ----
	inline constexpr double kMicrosecondsToMilliseconds = 1.0 / 1000.0;

	inline constexpr double MicrosecondsToMilliseconds(double microseconds) noexcept
	{
		return microseconds * kMicrosecondsToMilliseconds;
	}

	// Sum of immediate children's average durations, in microseconds.
	double SumChildAverageMicroseconds(const std::vector<ProfilerSnapshotNode>& children) noexcept;

	// Inclusive minus child sum, clamped to zero. The clamp can hide measurement
	// inconsistency (concurrent/overlapping children, EMA desync); callers that
	// need to flag this should use ComputeExclusiveWithStatus.
	double ComputeExclusiveMicroseconds(const ProfilerSnapshotNode& node) noexcept;

	struct ExclusiveResult
	{
		double ExclusiveMicroseconds = 0.0;
		double InclusiveMicroseconds = 0.0;
		double ChildSumMicroseconds = 0.0;
		// True when the child sum exceeded the inclusive average by more than the
		// caller's tolerance (currently 1Âµs). Indicates the data is unreliable for
		// exclusive-time analysis (typically async/overlapping children).
		bool WasClampedToZero = false;
	};

	ExclusiveResult ComputeExclusiveWithStatus(const ProfilerSnapshotNode& node) noexcept;

	// ---- Tree traversal ----
	const ProfilerSnapshotNode* FindNodeByName(const std::vector<ProfilerSnapshotNode>& nodes, std::string_view name) noexcept;

	const ProfilerSnapshotNode* FindNodeInBucket(const std::vector<const ProfilerSnapshotNode*>& bucket, std::string_view name) noexcept;

	// ---- Naming helpers ----
	// Strips a `[Kind #] ` prefix (GPU) or dotted namespace (CPU) so the remaining
	// label reads cleanly. The result aliases into the input string.
	std::string_view ShortenScopeName(std::string_view fullName) noexcept;

	// Returns the module/category name used to group sibling scopes:
	//   - CPU `Renderer.RecordFrame` â†’ `Renderer`
	//   - GPU `[Graphics 0] DepthPass` â†’ `Graphics`
	//   - Otherwise the input is returned unchanged.
	std::string_view ExtractModuleName(std::string_view scopeName) noexcept;

	// Renders a thread label like `Render Thread (TID 12345)` or `Thread 12345`.
	// The buffer must hold at least 96 bytes for the longest expected names.
	void FormatThreadLabel(char* buffer, std::size_t bufferSize, const Diagnostics::ProfilerThreadSnapshot& thread) noexcept;
}
