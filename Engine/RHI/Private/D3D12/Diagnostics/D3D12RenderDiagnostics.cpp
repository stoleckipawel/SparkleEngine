#include "PCH.h"

#include "D3D12/Diagnostics/D3D12RenderDiagnostics.h"

#include "Commands/RenderCommandList.h"
#include "Diagnostics/RhiDiagnosticsComposition.h"
#include "Diagnostics/RhiTimestampQueryAllocator.h"

#include "Frame/RhiFrameConstants.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "Interop/RhiInteropService.h"

#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

static const auto g_d3d12RenderDiagnosticsLogger = Logging::GetOrCreateLogger("RHI.D3D12.Diagnostics");

class D3D12RenderObjectDiagnostics final : public RenderObjectDiagnostics
{
public:
	bool SupportsObjectNames() const noexcept override { return true; }

	void SetDebugName(const RenderCommandList& commandList, std::wstring_view debugName) noexcept override
	{
		SetD3D12ObjectDebugName(
		    static_cast<ID3D12Object*>(commandList
		            .GetNativeHandle(
		                RhiNativeInteropRequest{
		                    .Consumer = ERhiNativeInteropConsumer::Diagnostics,
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
	D3D12RenderTimingDiagnostics(D3D12Rhi& rhi, std::uint32_t maximumFramesInFlight) noexcept :
	    m_rhi(rhi),
	    m_maximumFramesInFlight(maximumFramesInFlight),
	    m_poolStates(static_cast<std::size_t>(maximumFramesInFlight) * RhiQueueTypeCount),
	    m_queryAllocator(maximumFramesInFlight * static_cast<std::uint32_t>(RhiQueueTypeCount), kQueriesPerQueuePerFrame)
	{
		Initialize();
	}

	bool SupportsTimestampQueries() const noexcept override
	{
		return !m_poolStates.empty() && m_poolStates.front().TimestampFrequencyHz != 0;
	}

	RhiTimestampQueryHandle AllocateTimestampQuery(ERhiQueueType queueType) override
	{
		if (!IsRhiQueueTypeValid(queueType))
		{
			Diagnostics::Fatal(g_d3d12RenderDiagnosticsLogger, __FILE__, __LINE__, "Timestamp query requested for an invalid D3D12 queue.");
		}

		const std::uint32_t frameIndex = m_rhi.GetCurrentFrameIndex();
		if (frameIndex >= m_maximumFramesInFlight)
		{
			Diagnostics::Fatal(
			    g_d3d12RenderDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    "D3D12 timestamp query addressed an invalid frame slot.");
		}

		const std::uint32_t poolIndex = GetPoolIndex(frameIndex, queueType);
		const RhiTimestampQueryHandle query = m_queryAllocator.Allocate(poolIndex);
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		m_poolStates[location.PoolIndex].MappedReadback[location.QueryIndex] = 0;
		return query;
	}

	void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		m_poolStates[location.PoolIndex].MappedReadback[location.QueryIndex] = 0;
		m_queryAllocator.Release(query);
	}

	bool WriteTimestamp(RenderCommandList& commandList, RhiTimestampQueryHandle query) noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		PoolTimingState& poolState = m_poolStates[location.PoolIndex];
		if (commandList.GetQueueType() != poolState.QueueType)
		{
			Diagnostics::Fatal(
			    g_d3d12RenderDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    "D3D12 timestamp query was written on a different queue than it was allocated for.");
		}

		ID3D12GraphicsCommandList* const nativeCommandList = D3D12TypeConversions::ToGraphicsCommandList(commandList.GetNativeHandle(
		    RhiNativeInteropRequest{.Consumer = ERhiNativeInteropConsumer::Diagnostics, .Reason = "Write D3D12 timestamp query"}));
		if (nativeCommandList == nullptr)
		{
			Diagnostics::Fatal(g_d3d12RenderDiagnosticsLogger, __FILE__, __LINE__, "D3D12 timestamp query has no native command list.");
		}

		nativeCommandList->EndQuery(poolState.QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, location.QueryIndex);
		nativeCommandList->ResolveQueryData(
		    poolState.QueryHeap.Get(),
		    D3D12_QUERY_TYPE_TIMESTAMP,
		    location.QueryIndex,
		    1,
		    poolState.ReadbackAllocation->Resource.Get(),
		    static_cast<UINT64>(location.QueryIndex) * sizeof(std::uint64_t));
		return true;
	}

	bool TryResolveTimestamp(RhiTimestampQueryHandle query, std::uint64_t& outTicks) const noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		outTicks = m_poolStates[location.PoolIndex].MappedReadback[location.QueryIndex];
		return true;
	}

	double GetTimestampPeriodNanoseconds(RhiTimestampQueryHandle query) const noexcept override
	{
		const RhiTimestampQueryLocation location = m_queryAllocator.Resolve(query);
		return 1'000'000'000.0 / static_cast<double>(m_poolStates[location.PoolIndex].TimestampFrequencyHz);
	}
	std::uint32_t GetTimestampValidBits(RhiTimestampQueryHandle) const noexcept override { return 64; }

private:
	static constexpr std::uint32_t kQueriesPerQueuePerFrame = 4096;

	struct PoolTimingState
	{
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap;
		std::unique_ptr<D3D12GpuAllocationRecord> ReadbackAllocation;
		std::uint64_t* MappedReadback = nullptr;
		std::uint64_t TimestampFrequencyHz = 0;
		ERhiQueueType QueueType = ERhiQueueType::Graphics;
	};

	static std::uint32_t GetPoolIndex(std::uint32_t frameIndex, ERhiQueueType queueType) noexcept
	{
		return frameIndex * static_cast<std::uint32_t>(RhiQueueTypeCount) + static_cast<std::uint32_t>(RhiQueueTypeToIndex(queueType));
	}

	void Initialize() noexcept
	{
		if (m_rhi.GetDevice() == nullptr)
		{
			Diagnostics::Fatal(g_d3d12RenderDiagnosticsLogger, __FILE__, __LINE__, "Cannot initialize D3D12 timing without a device.");
		}

		for (std::uint32_t frameIndex = 0; frameIndex < m_maximumFramesInFlight; ++frameIndex)
		{
			for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
			{
				InitializePoolState(frameIndex, static_cast<ERhiQueueType>(queueIndex));
			}
		}
	}

	void InitializePoolState(std::uint32_t frameIndex, ERhiQueueType queueType) noexcept
	{
		PoolTimingState& poolState = m_poolStates[GetPoolIndex(frameIndex, queueType)];
		poolState.QueueType = queueType;
		if (FAILED(m_rhi.GetCommandQueue(queueType)->GetTimestampFrequency(&poolState.TimestampFrequencyHz))
		    || poolState.TimestampFrequencyHz == 0)
		{
			Diagnostics::Fatal(
			    g_d3d12RenderDiagnosticsLogger,
			    __FILE__,
			    __LINE__,
			    "D3D12 command queue does not expose a timestamp frequency.");
		}

		const D3D12_QUERY_HEAP_DESC queryHeapDesc{
		    .Type = queueType == ERhiQueueType::Copy ? D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP : D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
		    .Count = kQueriesPerQueuePerFrame,
		    .NodeMask = 0};
		CHECK(m_rhi.GetDevice()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(poolState.QueryHeap.ReleaseAndGetAddressOf())));

		const RhiBufferResourceDesc readbackBufferDesc{
		    .SizeInBytes = static_cast<std::uint64_t>(kQueriesPerQueuePerFrame) * sizeof(std::uint64_t),
		    .StrideInBytes = sizeof(std::uint64_t),
		    .AllowUnorderedAccess = false};
		const D3D12_RESOURCE_DESC nativeReadbackDesc = D3D12TypeConversions::BuildBufferResourceDesc(readbackBufferDesc);
		const std::wstring queueName = Strings::ToWide(std::string_view(RhiQueueTypeToString(queueType)));
		const std::wstring readbackName = std::wstring(L"D3D12TimestampReadback_") + queueName + L"_Frame" + std::to_wstring(frameIndex);
		auto readbackAllocation = m_rhi.GetMemoryAllocator().CreateBuffer(
		    nativeReadbackDesc,
		    D3D12_RESOURCE_STATE_COPY_DEST,
		    RhiMemoryCategory::Readback,
		    RhiMemoryResidencyClass::HostReadback,
		    readbackName);
		if (readbackAllocation == nullptr || readbackAllocation->Resource == nullptr)
		{
			Diagnostics::Fatal(g_d3d12RenderDiagnosticsLogger, __FILE__, __LINE__, "Failed to allocate a D3D12 timestamp readback buffer.");
		}

		if (FAILED(readbackAllocation->Resource->Map(0, nullptr, reinterpret_cast<void**>(&poolState.MappedReadback))))
		{
			Diagnostics::Fatal(g_d3d12RenderDiagnosticsLogger, __FILE__, __LINE__, "Failed to map a D3D12 timestamp readback buffer.");
		}
		readbackAllocation->IsMapped = true;
		readbackAllocation->CpuMappedAddress = poolState.MappedReadback;

		std::memset(poolState.MappedReadback, 0, static_cast<std::size_t>(readbackBufferDesc.SizeInBytes));
		poolState.ReadbackAllocation = std::move(readbackAllocation);
		const std::wstring queryHeapName = std::wstring(L"D3D12TimestampQueryHeap_") + queueName + L"_Frame" + std::to_wstring(frameIndex);
		poolState.QueryHeap->SetName(queryHeapName.c_str());
	}

	D3D12Rhi& m_rhi;
	std::uint32_t m_maximumFramesInFlight = 0;
	std::vector<PoolTimingState> m_poolStates;
	RhiTimestampQueryAllocator m_queryAllocator;
};

class D3D12RenderMessageDiagnostics final : public RenderMessageDiagnostics
{
public:
	explicit D3D12RenderMessageDiagnostics(D3D12Rhi& rhi) noexcept :
	    m_rhi(rhi)
	{
	}

	bool SupportsDebugMessages() const noexcept override { return m_rhi.SupportsDebugMessages(); }

	bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept override { return m_rhi.TryPopDebugMessage(outMessage); }

	void ClearMessages() noexcept override { m_rhi.ClearDebugMessages(); }

private:
	D3D12Rhi& m_rhi;
};

class D3D12RenderFailureDiagnostics final : public RenderFailureDiagnostics
{
public:
	explicit D3D12RenderFailureDiagnostics(D3D12Rhi& rhi) noexcept :
	    m_rhi(rhi)
	{
	}

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
	explicit D3D12RenderMemoryDiagnostics(D3D12GpuMemoryAllocator& allocator) noexcept :
	    m_allocator(allocator)
	{
	}

	bool SupportsBudgetQueries() const noexcept override { return m_allocator.SupportsBudgetQueries(); }

	bool SupportsDelayedDestructionTracking() const noexcept override { return false; }

	RhiMemoryUsageSnapshot GetLatestMemorySnapshot() const override { return m_allocator.CreateMemoryUsageSnapshot(); }

private:
	D3D12GpuMemoryAllocator& m_allocator;
};

std::unique_ptr<RenderDiagnostics> CreateD3D12RenderDiagnostics(D3D12Rhi& rhi, std::uint32_t maximumFramesInFlight)
{
	return CreateRhiDiagnosticsComposition(
	    std::make_unique<D3D12RenderObjectDiagnostics>(),
	    std::make_unique<D3D12RenderTimingDiagnostics>(rhi, maximumFramesInFlight),
	    std::make_unique<D3D12RenderMessageDiagnostics>(rhi),
	    std::make_unique<D3D12RenderFailureDiagnostics>(rhi),
	    std::make_unique<D3D12RenderMemoryDiagnostics>(rhi.GetMemoryAllocator()),
	    D3D12PixEvents::IsAvailable());
}
