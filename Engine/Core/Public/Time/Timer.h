#pragma once

#include "Core/Public/CoreAPI.h"

#include <atomic>
#include <chrono>
#include <cstdint>

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<double>;

enum class TimeUnit : uint8_t
{
	Seconds,
	Milliseconds,
	Microseconds,
	Nanoseconds
};

enum class TimeDomain : uint8_t
{
	Unscaled,
	Scaled
};

struct TimeInfo
{
	uint64_t frameIndex = 0;
	Duration unscaledTime = Duration::zero();
	Duration scaledTime = Duration::zero();
	Duration unscaledDelta = Duration::zero();
	double timeScale = 1.0;
	Duration scaledDelta = Duration::zero();
	bool bPaused = false;
};

class SPARKLE_CORE_API Timer final
{
  public:
	Timer() noexcept;
	~Timer() = default;

	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;
	Timer(Timer&&) = delete;
	Timer& operator=(Timer&&) = delete;

	void Tick() noexcept;

	TimeInfo GetTimeInfo() const noexcept { return m_timeInfo; }

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
