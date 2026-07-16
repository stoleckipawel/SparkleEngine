#include "PCH.h"

#include "D3D12/Diagnostics/D3D12RenderDiagnostics.h"

#include "Commands/RenderCommandList.h"

#include "Frame/RhiFrameConstants.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "Interop/RhiInteropService.h"

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class D3D12RenderObjectDiagnostics final : public RenderObjectDiagnostics
{
  public:
	bool SupportsObjectNames() const noexcept override { return true; }

	void SetDebugName(const RenderCommandList& commandList, std::wstring_view debugName) noexcept override
	{
		SetD3D12ObjectDebugName(
		    static_cast<ID3D12Object*>(commandList.GetNativeHandle(
		                                       RhiNativeInteropRequest{
		                                           .Consumer = ERhiNativeInteropConsumer::Validation,
		                                           .Reason = "Assign D3D12 command list debug name"})
		                                       .Value),
		    debugName);
	}

	void SetDebugName(RhiResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		SetD3D12ObjectDebugName(static_cast<ID3D12Object*>(resource.Value), debugName);
	}

	void SetDebugName(RhiOwnedMemoryBlockHandle memoryBlock, std::wstring_view debugName) noexcept override
	{
		D3D12GpuHeapRecord* const record = GetD3D12GpuHeapRecord(memoryBlock);
		if (record != nullptr)
		{
			record->DebugName = debugName;
		}
		SetD3D12ObjectDebugName(record != nullptr ? record->NativeHeap.Get() : nullptr, debugName);
	}

	void SetDebugName(RhiOwnedResourceHandle resource, std::wstring_view debugName) noexcept override
	{
		D3D12GpuAllocationRecord* const record = GetD3D12GpuAllocationRecord(resource);
		if (record != nullptr)
		{
			SetD3D12AllocationRecordDebugName(*record, debugName);
		}
	}

  private:
	static void SetD3D12ObjectDebugName(ID3D12Object* object, std::wstring_view debugName) noexcept
	{
		if (object == nullptr || debugName.empty())
		{
			return;
		}

		std::wstring ownedName(debugName);
		object->SetName(ownedName.c_str());
	}
};

class D3D12RenderTimingDiagnostics final : public RenderTimingDiagnostics
{
  public:
	explicit D3D12RenderTimingDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(&rhi) { Initialize(); }

	bool SupportsTimestampQueries() const noexcept override { return m_supportsTimestampQueries; }

	RhiTimestampQueryHandle AllocateTimestampQuery() override
	{
		if (!m_supportsTimestampQueries || m_rhi == nullptr || m_queryLocations.size() >= std::numeric_limits<std::uint32_t>::max() - 1)
		{
			return {};
		}

		const std::uint32_t frameIndex = m_rhi->GetCurrentFrameIndex();
		if (frameIndex >= m_frameStates.size())
		{
			return {};
		}

		FrameTimingState& frameState = m_frameStates[frameIndex];
		if (frameState.FreeQueryIndices.empty() || frameState.MappedReadback == nullptr)
		{
			return {};
		}

		const std::uint32_t queryIndex = frameState.FreeQueryIndices.back();
		frameState.FreeQueryIndices.pop_back();
		frameState.MappedReadback[queryIndex] = 0;

		std::uint32_t handleValue = m_nextHandleValue++;
		while (handleValue == 0 || m_queryLocations.find(handleValue) != m_queryLocations.end())
		{
			handleValue = m_nextHandleValue++;
		}

		m_queryLocations.emplace(handleValue, QueryLocation{.FrameIndex = frameIndex, .QueryIndex = queryIndex});
		return RhiTimestampQueryHandle{.Value = handleValue};
	}

	void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept override
	{
		const auto locationIt = m_queryLocations.find(query.Value);
		if (locationIt == m_queryLocations.end())
		{
			return;
		}

		const QueryLocation location = locationIt->second;
		if (location.FrameIndex < m_frameStates.size())
		{
			FrameTimingState& frameState = m_frameStates[location.FrameIndex];
			if (location.QueryIndex < frameState.QueryCount)
			{
				if (frameState.MappedReadback != nullptr)
				{
					frameState.MappedReadback[location.QueryIndex] = 0;
				}
				frameState.FreeQueryIndices.push_back(location.QueryIndex);
			}
		}

		m_queryLocations.erase(locationIt);
	}

	bool WriteTimestamp(RenderCommandList& commandList, RhiTimestampQueryHandle query) noexcept override
	{
		if (!m_supportsTimestampQueries)
		{
			return false;
		}

		const auto locationIt = m_queryLocations.find(query.Value);
		if (locationIt == m_queryLocations.end())
		{
			return false;
		}

		const QueryLocation location = locationIt->second;
		if (location.FrameIndex >= m_frameStates.size())
		{
			return false;
		}

		FrameTimingState& frameState = m_frameStates[location.FrameIndex];
		ID3D12GraphicsCommandList* const nativeCommandList =
		    D3D12TypeConversions::ToGraphicsCommandList(commandList.GetNativeHandle(
		        RhiNativeInteropRequest{
		            .Consumer = ERhiNativeInteropConsumer::Validation,
		            .Reason = "Write D3D12 timestamp query"}));
		ID3D12Resource* const readbackBuffer = frameState.ReadbackAllocation != nullptr
		                                           ? frameState.ReadbackAllocation->Resource.Get()
		                                           : nullptr;
		if (nativeCommandList == nullptr || frameState.QueryHeap == nullptr || readbackBuffer == nullptr)
		{
			return false;
		}

		nativeCommandList->EndQuery(frameState.QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, location.QueryIndex);
		nativeCommandList->ResolveQueryData(
		    frameState.QueryHeap.Get(),
		    D3D12_QUERY_TYPE_TIMESTAMP,
		    location.QueryIndex,
		    1,
		    readbackBuffer,
		    static_cast<UINT64>(location.QueryIndex) * sizeof(std::uint64_t));
		return true;
	}

	bool TryResolveTimestamp(RhiTimestampQueryHandle query, std::uint64_t& outTicks) const noexcept override
	{
		outTicks = 0;
		const auto locationIt = m_queryLocations.find(query.Value);
		if (!m_supportsTimestampQueries || locationIt == m_queryLocations.end())
		{
			return false;
		}

		const QueryLocation location = locationIt->second;
		if (location.FrameIndex >= m_frameStates.size())
		{
			return false;
		}

		const FrameTimingState& frameState = m_frameStates[location.FrameIndex];
		if (frameState.MappedReadback == nullptr || location.QueryIndex >= frameState.QueryCount)
		{
			return false;
		}

		outTicks = frameState.MappedReadback[location.QueryIndex];
		return true;
	}

	std::uint64_t GetTimestampFrequencyHz() const noexcept override { return m_timestampFrequencyHz; }

  private:
	static constexpr std::uint32_t kQueriesPerFrame = 4096;

	struct QueryLocation
	{
		std::uint32_t FrameIndex = 0;
		std::uint32_t QueryIndex = 0;
	};

	struct FrameTimingState
	{
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap;
		std::unique_ptr<D3D12GpuAllocationRecord> ReadbackAllocation;
		std::uint64_t* MappedReadback = nullptr;
		std::vector<std::uint32_t> FreeQueryIndices;
		std::uint32_t QueryCount = 0;
	};

	void Initialize() noexcept
	{
		if (m_rhi == nullptr || m_rhi->GetDevice() == nullptr || m_rhi->GetCommandQueue() == nullptr)
		{
			return;
		}

		UINT64 timestampFrequency = 0;
		if (FAILED(m_rhi->GetCommandQueue()->GetTimestampFrequency(&timestampFrequency)) || timestampFrequency == 0)
		{
			return;
		}

		for (std::uint32_t frameIndex = 0; frameIndex < m_frameStates.size(); ++frameIndex)
		{
			if (!InitializeFrameState(frameIndex))
			{
				return;
			}
		}

		m_timestampFrequencyHz = timestampFrequency;
		m_supportsTimestampQueries = true;
	}

	bool InitializeFrameState(std::uint32_t frameIndex) noexcept
	{
		FrameTimingState& frameState = m_frameStates[frameIndex];
		const D3D12_QUERY_HEAP_DESC queryHeapDesc{.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP, .Count = kQueriesPerFrame, .NodeMask = 0};
		if (FAILED(m_rhi->GetDevice()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(frameState.QueryHeap.ReleaseAndGetAddressOf()))))
		{
			return false;
		}

		const RhiBufferResourceDesc readbackBufferDesc{
		    .SizeInBytes = static_cast<std::uint64_t>(kQueriesPerFrame) * sizeof(std::uint64_t),
		    .StrideInBytes = sizeof(std::uint64_t),
		    .AllowUnorderedAccess = false};
		const D3D12_RESOURCE_DESC nativeReadbackDesc = D3D12TypeConversions::BuildBufferResourceDesc(readbackBufferDesc);
		std::wstring readbackName = std::wstring(L"D3D12TimestampReadback_Frame") + std::to_wstring(frameIndex);
		auto readbackAllocation = m_rhi->GetMemoryAllocator().CreateBuffer(
		    nativeReadbackDesc,
		    D3D12_RESOURCE_STATE_COPY_DEST,
		    RhiMemoryCategory::Readback,
		    RhiMemoryResidencyClass::HostReadback,
		    readbackName);
		if (readbackAllocation == nullptr || readbackAllocation->Resource == nullptr)
		{
			return false;
		}

		if (FAILED(readbackAllocation->Resource->Map(0, nullptr, reinterpret_cast<void**>(&frameState.MappedReadback))))
		{
			frameState.MappedReadback = nullptr;
			return false;
		}
		readbackAllocation->IsMapped = true;
		readbackAllocation->CpuMappedAddress = frameState.MappedReadback;

		std::memset(frameState.MappedReadback, 0, static_cast<std::size_t>(readbackBufferDesc.SizeInBytes));
		frameState.ReadbackAllocation = std::move(readbackAllocation);
		frameState.QueryCount = kQueriesPerFrame;
		frameState.FreeQueryIndices.reserve(kQueriesPerFrame);
		for (std::uint32_t queryIndex = kQueriesPerFrame; queryIndex > 0; --queryIndex)
		{
			frameState.FreeQueryIndices.push_back(queryIndex - 1);
		}

		std::wstring queryHeapName = std::wstring(L"D3D12TimestampQueryHeap_Frame") + std::to_wstring(frameIndex);
		frameState.QueryHeap->SetName(queryHeapName.c_str());
		return true;
	}

	D3D12Rhi* m_rhi = nullptr;
	std::array<FrameTimingState, RhiFrameConstants::FramesInFlight> m_frameStates;
	std::unordered_map<std::uint32_t, QueryLocation> m_queryLocations;
	std::uint32_t m_nextHandleValue = 1;
	std::uint64_t m_timestampFrequencyHz = 0;
	bool m_supportsTimestampQueries = false;
};

class D3D12RenderMessageDiagnostics final : public RenderMessageDiagnostics
{
  public:
	explicit D3D12RenderMessageDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(rhi) {}

	bool SupportsDebugMessages() const noexcept override { return m_rhi.SupportsDebugMessages(); }

	bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept override { return m_rhi.TryPopDebugMessage(outMessage); }

	void ClearMessages() noexcept override { m_rhi.ClearDebugMessages(); }

  private:
	D3D12Rhi& m_rhi;
};

class D3D12RenderFailureDiagnostics final : public RenderFailureDiagnostics
{
  public:
	explicit D3D12RenderFailureDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(rhi) {}

	bool SupportsLiveObjectReports() const noexcept override { return m_rhi.SupportsLiveObjectReports(); }

	bool SupportsCrashDiagnostics() const noexcept override { return m_rhi.SupportsCrashDiagnostics(); }

	void ReportLiveObjects() noexcept override { m_rhi.ReportLiveObjects(); }

	void CollectCrashDiagnostics() noexcept override { m_rhi.CollectCrashDiagnostics(); }

  private:
	D3D12Rhi& m_rhi;
};

class D3D12RenderMemoryDiagnostics final : public RenderMemoryDiagnostics
{
  public:
	explicit D3D12RenderMemoryDiagnostics(D3D12GpuMemoryAllocator& allocator) noexcept : m_allocator(allocator) {}

	bool SupportsBudgetQueries() const noexcept override { return m_allocator.SupportsBudgetQueries(); }

	bool SupportsDelayedDestructionTracking() const noexcept override { return false; }

	RhiMemoryUsageSnapshot GetLatestMemorySnapshot() const override { return m_allocator.CreateMemoryUsageSnapshot(); }

  private:
	D3D12GpuMemoryAllocator& m_allocator;
};

class D3D12RenderDiagnostics final : public RenderDiagnostics
{
  public:
	explicit D3D12RenderDiagnostics(D3D12Rhi& rhi) noexcept :
	    m_timingDiagnostics(rhi),
	    m_messageDiagnostics(rhi),
	    m_failureDiagnostics(rhi),
	    m_memoryDiagnostics(rhi.GetMemoryAllocator())
	{
	}

	RhiDiagnosticsCapabilities GetCapabilities() const noexcept override
	{
		return RhiDiagnosticsCapabilities{
		    .SupportsObjectNames = m_objectDiagnostics.SupportsObjectNames(),
		    .SupportsGpuEvents = D3D12PixEvents::IsAvailable(),
		    .SupportsTimestampQueries = m_timingDiagnostics.SupportsTimestampQueries(),
		    .SupportsDebugMessages = m_messageDiagnostics.SupportsDebugMessages(),
		    .SupportsLiveObjectReports = m_failureDiagnostics.SupportsLiveObjectReports(),
		    .SupportsCrashDiagnostics = m_failureDiagnostics.SupportsCrashDiagnostics(),
		    .SupportsMemoryDiagnostics = true,
		    .SupportsMemoryBudgetQueries = m_memoryDiagnostics.SupportsBudgetQueries()};
	}

	RenderObjectDiagnostics& GetObjectDiagnostics() noexcept override { return m_objectDiagnostics; }

	const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept override { return m_objectDiagnostics; }

	RenderTimingDiagnostics* GetTimingDiagnostics() noexcept override
	{
		return m_timingDiagnostics.SupportsTimestampQueries() ? &m_timingDiagnostics : nullptr;
	}

	const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept override
	{
		return m_timingDiagnostics.SupportsTimestampQueries() ? &m_timingDiagnostics : nullptr;
	}

	RenderMessageDiagnostics* GetMessageDiagnostics() noexcept override
	{
		return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
	}

	const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept override
	{
		return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
	}

	RenderFailureDiagnostics* GetFailureDiagnostics() noexcept override
	{
		return (m_failureDiagnostics.SupportsLiveObjectReports() || m_failureDiagnostics.SupportsCrashDiagnostics()) ? &m_failureDiagnostics
		                                                                                                             : nullptr;
	}

	const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept override
	{
		return (m_failureDiagnostics.SupportsLiveObjectReports() || m_failureDiagnostics.SupportsCrashDiagnostics()) ? &m_failureDiagnostics
		                                                                                                             : nullptr;
	}

	RenderMemoryDiagnostics* GetMemoryDiagnostics() noexcept override { return &m_memoryDiagnostics; }

	const RenderMemoryDiagnostics* GetMemoryDiagnostics() const noexcept override { return &m_memoryDiagnostics; }

  private:
	D3D12RenderObjectDiagnostics m_objectDiagnostics;
	D3D12RenderTimingDiagnostics m_timingDiagnostics;
	D3D12RenderMessageDiagnostics m_messageDiagnostics;
	D3D12RenderFailureDiagnostics m_failureDiagnostics;
	D3D12RenderMemoryDiagnostics m_memoryDiagnostics;
};

std::unique_ptr<RenderDiagnostics> CreateD3D12RenderDiagnostics(D3D12Rhi& rhi)
{
	return std::make_unique<D3D12RenderDiagnostics>(rhi);
}
