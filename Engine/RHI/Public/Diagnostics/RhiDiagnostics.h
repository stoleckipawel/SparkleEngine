#pragma once

#include "../Commands/RhiQueue.h"
#include "../Memory/RhiMemoryDiagnostics.h"
#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string>
#include <string_view>

class RenderCommandList;

struct RhiDiagnosticLabelColor
{
	std::uint8_t Red = 255;
	std::uint8_t Green = 255;
	std::uint8_t Blue = 255;
	std::uint8_t Alpha = 255;
};

struct RhiTimestampQueryHandle
{
	std::uint32_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

enum class ERhiDiagnosticMessageSeverity : std::uint8_t
{
	Verbose = 0,
	Info = 1,
	Warning = 2,
	Error = 3,
	Fatal = 4,
};

enum class ERhiDiagnosticMessageCategory : std::uint8_t
{
	General = 0,
	Validation = 1,
	Performance = 2,
	ResourceLifetime = 3,
	Shader = 4,
	Driver = 5,
	Capture = 6,
};

struct RhiDiagnosticMessage
{
	ERhiDiagnosticMessageSeverity Severity = ERhiDiagnosticMessageSeverity::Info;
	ERhiDiagnosticMessageCategory Category = ERhiDiagnosticMessageCategory::General;
	std::string Text = {};
};

struct RhiDiagnosticsCapabilities
{
	bool SupportsObjectNames = false;
	bool SupportsGpuEvents = false;
	bool SupportsTimestampQueries = false;
	bool SupportsDebugMessages = false;
	bool SupportsLiveObjectReports = false;
	bool SupportsCrashDiagnostics = false;
	bool SupportsMemoryDiagnostics = false;
	bool SupportsMemoryBudgetQueries = false;
};

class SPARKLE_RHI_API RenderObjectDiagnostics
{
  public:
	virtual ~RenderObjectDiagnostics() noexcept = default;

	virtual bool SupportsObjectNames() const noexcept = 0;
	virtual void SetDebugName(const RenderCommandList& commandList, std::wstring_view debugName) noexcept = 0;
	virtual void SetDebugName(RhiResourceHandle resource, std::wstring_view debugName) noexcept = 0;
	virtual void SetDebugName(RhiOwnedMemoryBlockHandle memoryBlock, std::wstring_view debugName) noexcept = 0;
	virtual void SetDebugName(RhiOwnedResourceHandle resource, std::wstring_view debugName) noexcept = 0;
};

class SPARKLE_RHI_API RenderTimingDiagnostics
{
  public:
	virtual ~RenderTimingDiagnostics() noexcept = default;

	virtual bool SupportsTimestampQueries() const noexcept = 0;
	virtual RhiTimestampQueryHandle AllocateTimestampQuery(ERhiQueueType queueType) = 0;
	virtual void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept = 0;
	virtual bool WriteTimestamp(RenderCommandList& commandList, RhiTimestampQueryHandle query) noexcept = 0;
	virtual bool TryResolveTimestamp(RhiTimestampQueryHandle query, std::uint64_t& outTicks) const noexcept = 0;
	virtual double GetTimestampPeriodNanoseconds(RhiTimestampQueryHandle query) const noexcept = 0;
	virtual std::uint32_t GetTimestampValidBits(RhiTimestampQueryHandle query) const noexcept = 0;
};

class SPARKLE_RHI_API RenderMessageDiagnostics
{
  public:
	virtual ~RenderMessageDiagnostics() noexcept = default;

	virtual bool SupportsDebugMessages() const noexcept = 0;
	virtual bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept = 0;
	virtual void ClearMessages() noexcept = 0;
};

class SPARKLE_RHI_API RenderFailureDiagnostics
{
  public:
	virtual ~RenderFailureDiagnostics() noexcept = default;

	virtual bool SupportsLiveObjectReports() const noexcept = 0;
	virtual bool SupportsCrashDiagnostics() const noexcept = 0;
	virtual void ReportLiveObjects() noexcept = 0;
	virtual void CollectCrashDiagnostics() noexcept = 0;
};

class SPARKLE_RHI_API RenderDiagnostics
{
  public:
	virtual ~RenderDiagnostics() noexcept = default;

	virtual RhiDiagnosticsCapabilities GetCapabilities() const noexcept = 0;
	virtual RenderObjectDiagnostics& GetObjectDiagnostics() noexcept = 0;
	virtual const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept = 0;
	virtual RenderTimingDiagnostics* GetTimingDiagnostics() noexcept = 0;
	virtual const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept = 0;
	virtual RenderMessageDiagnostics* GetMessageDiagnostics() noexcept = 0;
	virtual const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept = 0;
	virtual RenderFailureDiagnostics* GetFailureDiagnostics() noexcept = 0;
	virtual const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept = 0;
	virtual RenderMemoryDiagnostics* GetMemoryDiagnostics() noexcept = 0;
	virtual const RenderMemoryDiagnostics* GetMemoryDiagnostics() const noexcept = 0;
};
