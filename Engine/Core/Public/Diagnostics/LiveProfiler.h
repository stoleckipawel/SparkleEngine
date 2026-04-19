#pragma once

#include "Core/Public/CoreAPI.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Engine::Diagnostics
{
	// Immutable snapshot node consumed by the editor profiler panel.
	// The tree is owned by value so the consumer can read it without locking.
	struct SPARKLE_CORE_API ProfilerSnapshotNode final
	{
		std::string Name;
		double LastDurationMicroseconds = 0.0;
		double AverageDurationMicroseconds = 0.0;
		double MaxDurationMicroseconds = 0.0;
		std::uint64_t TotalCallCount = 0;
		std::vector<ProfilerSnapshotNode> Children;
	};

	struct SPARKLE_CORE_API ProfilerThreadSnapshot final
	{
		std::uint32_t ThreadId = 0;
		std::string ThreadName;
		std::vector<ProfilerSnapshotNode> Roots;
	};

	struct SPARKLE_CORE_API ProfilerSnapshot final
	{
		std::vector<ProfilerThreadSnapshot> CpuThreads;
		std::vector<ProfilerSnapshotNode> GpuRoots;
	};

	// Single-responsibility live profiling store: keeps an in-memory hierarchical
	// view of CPU and GPU timings for the editor. Performs no I/O and no logging.
	//
	// Producers:
	//   - ScopedTrace pushes CPU scopes through BeginCpuScope/EndCpuScope.
	//   - Renderer publishes resolved GPU timings through SubmitGpuFrame.
	//
	// Consumer:
	//   - Editor reads via CaptureSnapshot(); call once per UI build.
	class SPARKLE_CORE_API LiveProfiler final
	{
	  public:
		struct GpuTimingEntry final
		{
			std::string_view Label;
			std::uint64_t DurationMicroseconds = 0;
			std::uint16_t Depth = 0;
		};

		static LiveProfiler& Get() noexcept;

		void SetEnabled(bool enabled) noexcept { m_enabled.store(enabled, std::memory_order_release); }
		bool IsEnabled() const noexcept { return m_enabled.load(std::memory_order_acquire); }

		// CPU producer surface. Both calls must be balanced and same-thread.
		void BeginCpuScope(std::string_view name) noexcept;
		void EndCpuScope(std::uint64_t durationMicroseconds) noexcept;

		// GPU producer surface. Entries must be ordered by submission and carry
		// nesting depth so the profiler can rebuild the hierarchy without parsing
		// label strings.
		void SubmitGpuFrame(const GpuTimingEntry* entries, std::size_t count) noexcept;

		// Consumer surface.
		ProfilerSnapshot CaptureSnapshot() const;

		// Drops accumulated state. Useful when a level changes or for tests.
		void Reset() noexcept;

	  private:
		LiveProfiler() noexcept;
		~LiveProfiler() noexcept;

		LiveProfiler(const LiveProfiler&) = delete;
		LiveProfiler& operator=(const LiveProfiler&) = delete;
		LiveProfiler(LiveProfiler&&) = delete;
		LiveProfiler& operator=(LiveProfiler&&) = delete;

		struct State;
		std::unique_ptr<State> m_state;
		std::atomic<bool> m_enabled{true};
	};
}
