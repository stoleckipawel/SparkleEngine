#include "PCH.h"
#include "Timer.h"

Timer::Timer() noexcept : 
m_start{Clock::now()}, 
m_last{m_start} 
{
	
}

void Timer::Tick() noexcept
{
	// Read current time and compute raw delta.
	const TimePoint now = Clock::now();
	m_unscaledDelta = std::chrono::duration_cast<Duration>(now - m_last);
	m_last = now;

	// Accumulate totals.
	m_unscaledTotal += m_unscaledDelta;

	const bool bPaused = m_bPaused.load(std::memory_order_relaxed);
	if (!bPaused)
	{
		const Duration scaled{m_unscaledDelta.count() * m_timeScale};
		m_scaledTotal += scaled;
	}

	// Advance frame counter.
	++m_frameCount;

	// Update snapshot for consumers.
	m_timeInfo.frameIndex = m_frameCount;
	m_timeInfo.unscaledTime = m_unscaledTotal;
	m_timeInfo.scaledTime = m_scaledTotal;
	m_timeInfo.unscaledDelta = m_unscaledDelta;
	m_timeInfo.timeScale = m_timeScale;
	m_timeInfo.scaledDelta = bPaused ? Duration::zero() : Duration{m_unscaledDelta.count() * m_timeScale};
	m_timeInfo.bPaused = bPaused;
}

double Timer::ToUnit(Duration d, TimeUnit u) noexcept
{
	switch (u)
	{
		case TimeUnit::Seconds:
			return d.count();
		case TimeUnit::Milliseconds:
			return d.count() * 1e3;
		case TimeUnit::Microseconds:
			return d.count() * 1e6;
		case TimeUnit::Nanoseconds:
			return d.count() * 1e9;
	}
	return d.count();
}

double Timer::GetDelta(TimeDomain domain, TimeUnit unit) const noexcept
{
	const Duration delta = (domain == TimeDomain::Scaled) ? m_timeInfo.scaledDelta : m_unscaledDelta;
	return ToUnit(delta, unit);
}

double Timer::GetTotalTime(TimeDomain domain, TimeUnit unit) const noexcept
{
	const Duration total = (domain == TimeDomain::Scaled) ? m_scaledTotal : m_unscaledTotal;
	return ToUnit(total, unit);
}
