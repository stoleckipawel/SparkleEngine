// ============================================================================
// Timer.h
// ----------------------------------------------------------------------------
// Authoritative frame-timing service for the engine.
//
// USAGE:
//   Timer timer;  // Automatically initializes
//   // Each frame:
//   timer.Tick();
//   auto info = timer.GetTimeInfo();
//   double dt = timer.GetDelta(TimeDomain::Scaled);
//
// DESIGN:
//   - Provides both unscaled (wall) and scaled (game) time domains
//   - TimeInfo struct gives immutable snapshot of frame timing
//   - Supports pause/resume and time scaling for slow-mo effects
//
// NOTES:
//   - Owned by App, passed by reference to systems that need it
//   - Frame counter is 1-based
// ============================================================================

#pragma once

#include "Core/Public/CoreAPI.h"

#include <atomic>
#include <chrono>
#include <cstdint>

// ========================================================================
// Internal Clock Types
// ========================================================================

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<double>;  ///< Seconds, double-precision

// Supported time display units. Default is Milliseconds (convenient for UI).
enum class TimeUnit : uint8_t
{
	Seconds,
	Milliseconds,
	Microseconds,
	Nanoseconds
};

// Which clock domain to query.
enum class TimeDomain : uint8_t
{
	Unscaled,  // wall/real time (ignores timeScale, continues when paused)
	Scaled     // game time (multiplied by timeScale, stops when paused)
};

// -------------------------------------------------------------------------
// TimeInfo: immutable snapshot of frame timing. Cheap to copy by value.
// -------------------------------------------------------------------------
struct TimeInfo
{
	uint64_t frameIndex = 0;                    // 1-based frame counter
	Duration unscaledTime = Duration::zero();   // total wall time since init
	Duration scaledTime = Duration::zero();     // total scaled/game time since init (stops when paused)
	Duration unscaledDelta = Duration::zero();  // raw delta this frame (seconds)
	double timeScale = 1.0;                     // game-time multiplier
	Duration scaledDelta = Duration::zero();    // delta * timeScale (0 if paused)
	bool bPaused = false;                       // true when scaled time is paused
};

// -------------------------------------------------------------------------
// Timer: authoritative frame-timing service for the engine.
// -------------------------------------------------------------------------
class SPARKLE_CORE_API Timer final
{
  public:
	Timer() noexcept;
	~Timer() = default;

	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	Timer(Timer&&) = delete;
	Timer& operator=(Timer&&) = delete;

	// Advance clocks. Call once per rendered frame.
	void Tick() noexcept;

	// Immutable snapshot of current frame timing.
	TimeInfo GetTimeInfo() const noexcept { return m_timeInfo; }

	// Frame counter (1-based, incremented each Tick).
	uint64_t GetFrameCount() const noexcept { return m_frameCount; }

	double GetDelta(TimeDomain domain, TimeUnit unit = TimeUnit::Milliseconds) const noexcept;
	double GetTotalTime(TimeDomain domain, TimeUnit unit = TimeUnit::Milliseconds) const noexcept;

	void SetTimeScale(double scale) noexcept { m_timeScale = scale; }
	double GetTimeScale() const noexcept { return m_timeScale; }

	void Pause() noexcept { m_bPaused.store(true, std::memory_order_relaxed); }
	void Resume() noexcept { m_bPaused.store(false, std::memory_order_relaxed); }
	bool IsPaused() const noexcept { return m_bPaused.load(std::memory_order_relaxed); }

	struct Stopwatch
	{
		TimePoint start;

		Stopwatch() noexcept : start(Clock::now()) {}
		void Reset() noexcept { start = Clock::now(); }

		Duration Elapsed() const noexcept { return std::chrono::duration_cast<Duration>(Clock::now() - start); }
		double ElapsedSeconds() const noexcept { return Elapsed().count(); }
		double ElapsedMillis() const noexcept { return Elapsed().count() * 1e3; }
	};

  private:
	static double ToUnit(Duration d, TimeUnit u) noexcept;

	TimePoint m_start{};
	TimePoint m_last{};
	Duration m_unscaledDelta{1.0 / 60.0};
	Duration m_unscaledTotal{Duration::zero()};
	Duration m_scaledTotal{Duration::zero()};
	double m_timeScale{1.0};
	std::atomic<bool> m_bPaused{false};
	uint64_t m_frameCount{0};
	TimeInfo m_timeInfo{};
};
