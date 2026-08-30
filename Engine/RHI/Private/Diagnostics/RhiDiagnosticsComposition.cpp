#include "PCH.h"

#include "Diagnostics/RhiDiagnosticsComposition.h"

#include <cassert>
#include <utility>

class RhiDiagnosticsComposition final : public RenderDiagnostics
{
public:
	RhiDiagnosticsComposition(
	    std::unique_ptr<RenderObjectDiagnostics> objectDiagnostics,
	    std::unique_ptr<RenderTimingDiagnostics> timingDiagnostics,
	    std::unique_ptr<RenderMessageDiagnostics> messageDiagnostics,
	    std::unique_ptr<RenderFailureDiagnostics> failureDiagnostics,
	    std::unique_ptr<RenderMemoryDiagnostics> memoryDiagnostics,
	    bool supportsGpuEvents) noexcept :
	    m_objectDiagnostics(std::move(objectDiagnostics)),
	    m_timingDiagnostics(std::move(timingDiagnostics)),
	    m_messageDiagnostics(std::move(messageDiagnostics)),
	    m_failureDiagnostics(std::move(failureDiagnostics)),
	    m_memoryDiagnostics(std::move(memoryDiagnostics)),
	    m_supportsGpuEvents(supportsGpuEvents)
	{
		assert(m_objectDiagnostics != nullptr);
	}

	RhiDiagnosticsCapabilities GetCapabilities() const noexcept override
	{
		return RhiDiagnosticsCapabilities{
		    .SupportsObjectNames = m_objectDiagnostics->SupportsObjectNames(),
		    .SupportsGpuEvents = m_supportsGpuEvents,
		    .SupportsTimestampQueries = GetTimingDiagnostics() != nullptr,
		    .SupportsDebugMessages = GetMessageDiagnostics() != nullptr,
		    .SupportsLiveObjectReports = m_failureDiagnostics != nullptr && m_failureDiagnostics->SupportsLiveObjectReports(),
		    .SupportsCrashDiagnostics = m_failureDiagnostics != nullptr && m_failureDiagnostics->SupportsCrashDiagnostics(),
		    .SupportsMemoryDiagnostics = m_memoryDiagnostics != nullptr,
		    .SupportsMemoryBudgetQueries = m_memoryDiagnostics != nullptr && m_memoryDiagnostics->SupportsBudgetQueries()};
	}

	RenderObjectDiagnostics& GetObjectDiagnostics() noexcept override { return *m_objectDiagnostics; }
	const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept override { return *m_objectDiagnostics; }

	RenderTimingDiagnostics* GetTimingDiagnostics() noexcept override
	{
		return m_timingDiagnostics != nullptr && m_timingDiagnostics->SupportsTimestampQueries() ? m_timingDiagnostics.get() : nullptr;
	}

	const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept override
	{
		return m_timingDiagnostics != nullptr && m_timingDiagnostics->SupportsTimestampQueries() ? m_timingDiagnostics.get() : nullptr;
	}

	RenderMessageDiagnostics* GetMessageDiagnostics() noexcept override
	{
		return m_messageDiagnostics != nullptr && m_messageDiagnostics->SupportsDebugMessages() ? m_messageDiagnostics.get() : nullptr;
	}

	const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept override
	{
		return m_messageDiagnostics != nullptr && m_messageDiagnostics->SupportsDebugMessages() ? m_messageDiagnostics.get() : nullptr;
	}

	RenderFailureDiagnostics* GetFailureDiagnostics() noexcept override
	{
		return HasFailureDiagnostics() ? m_failureDiagnostics.get() : nullptr;
	}

	const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept override
	{
		return HasFailureDiagnostics() ? m_failureDiagnostics.get() : nullptr;
	}

	RenderMemoryDiagnostics* GetMemoryDiagnostics() noexcept override { return m_memoryDiagnostics.get(); }
	const RenderMemoryDiagnostics* GetMemoryDiagnostics() const noexcept override { return m_memoryDiagnostics.get(); }

private:
	bool HasFailureDiagnostics() const noexcept
	{
		return m_failureDiagnostics != nullptr
		    && (m_failureDiagnostics->SupportsLiveObjectReports() || m_failureDiagnostics->SupportsCrashDiagnostics());
	}

	std::unique_ptr<RenderObjectDiagnostics> m_objectDiagnostics;
	std::unique_ptr<RenderTimingDiagnostics> m_timingDiagnostics;
	std::unique_ptr<RenderMessageDiagnostics> m_messageDiagnostics;
	std::unique_ptr<RenderFailureDiagnostics> m_failureDiagnostics;
	std::unique_ptr<RenderMemoryDiagnostics> m_memoryDiagnostics;
	bool m_supportsGpuEvents = false;
};

std::unique_ptr<RenderDiagnostics> CreateRhiDiagnosticsComposition(
    std::unique_ptr<RenderObjectDiagnostics> objectDiagnostics,
    std::unique_ptr<RenderTimingDiagnostics> timingDiagnostics,
    std::unique_ptr<RenderMessageDiagnostics> messageDiagnostics,
    std::unique_ptr<RenderFailureDiagnostics> failureDiagnostics,
    std::unique_ptr<RenderMemoryDiagnostics> memoryDiagnostics,
    bool supportsGpuEvents)
{
	return std::make_unique<RhiDiagnosticsComposition>(
	    std::move(objectDiagnostics),
	    std::move(timingDiagnostics),
	    std::move(messageDiagnostics),
	    std::move(failureDiagnostics),
	    std::move(memoryDiagnostics),
	    supportsGpuEvents);
}
