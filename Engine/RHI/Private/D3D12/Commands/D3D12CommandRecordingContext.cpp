#include "PCH.h"

#include "D3D12/Commands/D3D12CommandRecordingContext.h"

#include "Commands/RhiCommandRecordingLeaseAccess.h"
#include "D3D12/Commands/D3D12CommandQueue.h"
#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Device/D3D12Rhi.h"

#include <cassert>
#include <exception>
#include <format>

D3D12CommandRecordingContext::D3D12CommandRecordingContext(
    D3D12Rhi& rhi,
    D3D12RenderHardwareInterface& hardwareInterface,
    D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
	m_rhi(&rhi), m_hardwareInterface(&hardwareInterface), m_descriptorHeapManager(&descriptorHeapManager)
{
	InitializeSlots();
}

D3D12CommandRecordingContext::~D3D12CommandRecordingContext() noexcept
{
	ReleaseDescriptorPages();
}

void D3D12CommandRecordingContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		frameState.CurrentLease.reset();
		WaitForFrameStateRetirement(frameState);

		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			ResetSlot(*slot);
		}
	}
}

RhiCommandRecordingLease D3D12CommandRecordingContext::Acquire(
    ERhiQueueType queueType,
    std::uint32_t frameIndex,
    RhiCommandRecordingOwner owner) noexcept
{
	CommandSlot& slot = AcquireSlot(queueType, frameIndex);
	const RhiSubmissionToken reusableAfter = slot.RetirementToken;
	slot.RetirementToken = {};
	slot.RecordingOwner = owner;
	slot.CommandList->SetRecordingOwner(owner);
	slot.RecordingThread = {};
	slot.State = SlotState::Recording;

	const RhiCommandRecordingLeaseInitialization initialization{
	    .BackendState = &slot,
	    .CommandList = slot.CommandList.get(),
	    .QueueType = queueType,
	    .FrameSlot = slot.FrameSlot,
	    .ContextId = RhiCommandRecordingContextId{.Value = slot.ContextIndex},
	    .Owner = owner,
	    .UploadPage = RhiCommandRecordingUploadPage{.CapacityInBytes = slot.UploadPage.GetCapacityInBytes()},
	    .DescriptorPage = RhiCommandRecordingDescriptorPage{
	        .CpuBase = RhiCpuDescriptorHandle{.Value = slot.DescriptorPage.GetCPU().ptr},
	        .GpuBase = RhiGpuDescriptorHandle{.Value = slot.DescriptorPage.GetGPU().ptr},
	        .Capacity = DescriptorPageCapacity},
	    .RetirementToken = reusableAfter,
	    .Begin = &BeginLease,
	    .Close = &CloseLease,
	    .Release = &ReleaseLease,
	    .AllocateDescriptors = &AllocateLeaseDescriptors};
	return RhiCommandRecordingLeaseAccess::Create(initialization);
}

RhiSubmissionToken D3D12CommandRecordingContext::Submit(
    RhiCommandRecordingLease&& lease,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	std::array<RhiCommandRecordingLease, 1> leases;
	leases.front() = std::move(lease);
	return SubmitBatch(leases, waitTokens);
}

RhiSubmissionToken D3D12CommandRecordingContext::SubmitBatch(
    std::span<RhiCommandRecordingLease> leases,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	if (leases.empty() ||
	    leases.size() > MaximumContextsPerFrameQueue)
	{
		return {};
	}

	const ERhiQueueType queueType = leases.front().GetQueueType();
	std::array<CommandSlot*, MaximumContextsPerFrameQueue> slots{};
	std::array<ID3D12CommandList*, MaximumContextsPerFrameQueue> nativeCommandLists{};

	for (std::size_t index = 0; index < leases.size(); ++index)
	{
		if (!leases[index].IsClosed())
		{
			leases[index].Close();
		}

		CommandSlot* const slot =
		    ConsumeClosedLease(std::move(leases[index]));
		if (slot == nullptr ||
		    slot->QueueType != queueType)
		{
			return {};
		}

		slots[index] = slot;
		nativeCommandLists[index] = slot->NativeCommandList.Get();
	}

	const RhiSubmissionToken token = m_rhi->SubmitCommandLists(
	    queueType,
	    std::span<ID3D12CommandList* const>(
	        nativeCommandLists.data(),
	        leases.size()),
	    waitTokens);

	for (std::size_t index = 0; index < leases.size(); ++index)
	{
		ResolveSubmittedSlot(*slots[index], token);
	}

	return token;
}

RenderCommandList& D3D12CommandRecordingContext::BeginCurrentGraphicsCommandList(
    std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(ERhiQueueType::Graphics, frameIndex);
	frameState.CurrentLease.emplace(Acquire(ERhiQueueType::Graphics, frameIndex, RhiCommandRecordingOwner{}));
	return frameState.CurrentLease->GetCommandList();
}

RhiCommandRecordingLease
D3D12CommandRecordingContext::TakeCurrentGraphicsCommandRecordingLease(
    std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState =
	    GetQueueFrameState(ERhiQueueType::Graphics, frameIndex);
	if (!frameState.CurrentLease.has_value())
	{
		return {};
	}

	RhiCommandRecordingLease lease(
	    std::move(*frameState.CurrentLease));
	frameState.CurrentLease.reset();
	return lease;
}

RhiSubmissionToken D3D12CommandRecordingContext::SubmitCurrentGraphicsCommandList(
    std::uint32_t frameIndex,
    std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	RhiCommandRecordingLease lease =
	    TakeCurrentGraphicsCommandRecordingLease(frameIndex);
	if (!lease.IsValid())
	{
		return {};
	}

	return Submit(std::move(lease), waitTokens);
}

RenderCommandList& D3D12CommandRecordingContext::GetCurrentCommandList(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	RenderCommandList* const commandList = TryGetCurrentCommandList(queueType, frameIndex);
	assert(commandList != nullptr);
	return *commandList;
}

RenderCommandList* D3D12CommandRecordingContext::TryGetCurrentCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	return frameState.CurrentLease.has_value()
	           ? &frameState.CurrentLease->GetCommandList()
	           : nullptr;
}

D3D12CommandRecordingContext::QueueFrameState& D3D12CommandRecordingContext::GetQueueFrameState(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	return m_frames[frameIndex % m_frames.size()][RhiQueueTypeToIndex(queueType)];
}

D3D12CommandRecordingContext::CommandSlot& D3D12CommandRecordingContext::AcquireSlot(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		if (slot->State == SlotState::Available)
		{
			return *slot;
		}
	}

	FailExhausted(queueType, frameIndex);
}

void D3D12CommandRecordingContext::InitializeSlots()
{
	for (std::uint32_t frameIndex = 0; frameIndex < RhiFrameConstants::FramesInFlight; ++frameIndex)
	{
		for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
		{
			const ERhiQueueType queueType = static_cast<ERhiQueueType>(queueIndex);
			for (std::uint32_t contextIndex = 0; contextIndex < MaximumContextsPerFrameQueue; ++contextIndex)
			{
				(void)CreateSlot(queueType, frameIndex);
			}
		}
	}
}

D3D12CommandRecordingContext::CommandSlot& D3D12CommandRecordingContext::CreateSlot(
    ERhiQueueType queueType,
    std::uint32_t frameIndex)
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	const std::uint32_t contextIndex = static_cast<std::uint32_t>(frameState.Slots.size());
	auto slot = std::make_unique<CommandSlot>();

	slot->Owner = this;
	slot->QueueType = queueType;
	slot->FrameSlot = frameIndex % RhiFrameConstants::FramesInFlight;
	slot->ContextIndex = contextIndex;

	CreateNativeCommandObjects(*slot);
	InitializeSlotResources(*slot);
	NameSlotObjects(*slot);

	frameState.Slots.push_back(std::move(slot));
	return *frameState.Slots.back();
}

void D3D12CommandRecordingContext::CreateNativeCommandObjects(CommandSlot& slot)
{
	const D3D12_COMMAND_LIST_TYPE nativeType = D3D12CommandQueue::GetNativeCommandListType(slot.QueueType);
	CHECK(m_rhi->GetDevice()->CreateCommandAllocator(nativeType, IID_PPV_ARGS(slot.Allocator.ReleaseAndGetAddressOf())));
	CHECK(m_rhi->GetDevice()->CreateCommandList(
	    0,
	    nativeType,
	    slot.Allocator.Get(),
	    nullptr,
	    IID_PPV_ARGS(slot.NativeCommandList.ReleaseAndGetAddressOf())));
	CHECK(slot.NativeCommandList->Close());

	slot.CommandList =
	    std::make_unique<D3D12RenderCommandList>(*m_hardwareInterface, slot.NativeCommandList.Get(), slot.QueueType);
}

void D3D12CommandRecordingContext::InitializeSlotResources(CommandSlot& slot)
{
	slot.DescriptorPage =
	    m_descriptorHeapManager->AllocateContiguous(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescriptorPageCapacity);

	const std::wstring uploadName = std::format(
	    L"Sparkle {} Recording Upload Frame {} Context {}",
	    QueueTypeName(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	slot.UploadPage.Initialize(*m_rhi, UploadPageCapacityInBytes, uploadName.c_str());
	slot.CommandList->SetRecordingUploadPage(slot.UploadPage);
}

void D3D12CommandRecordingContext::NameSlotObjects(CommandSlot& slot) const noexcept
{
	const std::wstring allocatorName = std::format(
	    L"Sparkle {} Command Allocator Frame {} Context {}",
	    QueueTypeName(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	const std::wstring commandListName = std::format(
	    L"Sparkle {} Command List Frame {} Context {}",
	    QueueTypeName(slot.QueueType),
	    slot.FrameSlot,
	    slot.ContextIndex);
	(void)slot.Allocator->SetName(allocatorName.c_str());
	(void)slot.NativeCommandList->SetName(commandListName.c_str());
}

void D3D12CommandRecordingContext::WaitForFrameStateRetirement(
    const QueueFrameState& frameState) noexcept
{
	RhiSubmissionState retirement;
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		retirement.MarkUsed(slot->RetirementToken);
	}

	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = retirement.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
	{
		m_rhi->WaitForSubmission(tokens[tokenIndex]);
	}
}

void D3D12CommandRecordingContext::ResetSlot(CommandSlot& slot) noexcept
{
	assert(slot.State != SlotState::Recording);
	slot.UploadPage.Reset();
	slot.DescriptorOffset = 0;
	slot.RecordingOwner = {};
	slot.RecordingThread = {};
	slot.State = SlotState::Available;
}

void D3D12CommandRecordingContext::BeginSlot(CommandSlot& slot) noexcept
{
	assert(slot.State == SlotState::Recording);

	const std::thread::id thread = std::this_thread::get_id();
	if (slot.RecordingThread == thread)
	{
		return;
	}

	assert(slot.RecordingThread == std::thread::id{});
	CHECK(slot.Allocator->Reset());
	CHECK(slot.NativeCommandList->Reset(slot.Allocator.Get(), nullptr));

	slot.CommandList->ResetTrackedResources();
	slot.CommandList->ResetBoundState();
	slot.UploadPage.BeginRecording();
	slot.RecordingThread = thread;
}

void D3D12CommandRecordingContext::CloseSlot(CommandSlot& slot) noexcept
{
	assert(slot.State == SlotState::Recording);
	BeginSlot(slot);
	assert(slot.RecordingThread == std::this_thread::get_id());

	const HRESULT closeResult = slot.NativeCommandList->Close();
	if (FAILED(closeResult))
	{
		FailClose(slot, closeResult);
	}

	slot.UploadPage.EndRecording();
	slot.State = SlotState::Closed;
}

void D3D12CommandRecordingContext::ReleaseSlot(CommandSlot& slot) noexcept
{
	if (slot.State == SlotState::Recording)
	{
		CloseSlot(slot);
	}

	if (slot.State == SlotState::Closed)
	{
		slot.CommandList->ResetTrackedResources();
		ResetSlot(slot);
	}
}

D3D12CommandRecordingContext::CommandSlot*
D3D12CommandRecordingContext::ConsumeClosedLease(
    RhiCommandRecordingLease&& lease) noexcept
{
	const RhiCommandRecordingLeaseBackendState leaseState =
	    RhiCommandRecordingLeaseAccess::Consume(std::move(lease));
	auto* const slot =
	    static_cast<CommandSlot*>(leaseState.State);
	if (slot == nullptr ||
	    slot->Owner != this ||
	    slot->State != SlotState::Closed ||
	    !leaseState.Closed ||
	    leaseState.CommandList != slot->CommandList.get() ||
	    leaseState.QueueType != slot->QueueType ||
	    leaseState.FrameSlot != slot->FrameSlot ||
	    leaseState.ContextId.Value != slot->ContextIndex ||
	    leaseState.Owner.PartitionIndex !=
	        slot->RecordingOwner.PartitionIndex ||
	    leaseState.Owner.TaskIdentity !=
	        slot->RecordingOwner.TaskIdentity)
	{
		return nullptr;
	}

	return slot;
}

void D3D12CommandRecordingContext::ResolveSubmittedSlot(
    CommandSlot& slot,
    RhiSubmissionToken token) noexcept
{
	slot.CommandList->ResolveTrackedResources(token);
	slot.RetirementToken = token;
	slot.State = SlotState::Submitted;
}

RhiTransientDescriptorRange D3D12CommandRecordingContext::AllocateDescriptors(
    CommandSlot& slot,
    std::uint32_t count) noexcept
{
	assert(slot.State == SlotState::Recording);
	assert(slot.RecordingThread == std::this_thread::get_id());
	if (count == 0 || slot.DescriptorOffset + count > DescriptorPageCapacity)
	{
		return {};
	}

	const std::uint32_t offset = slot.DescriptorOffset;
	slot.DescriptorOffset += count;
	const std::uintptr_t cpu =
	    slot.DescriptorPage.GetCPU().ptr + static_cast<std::uintptr_t>(slot.DescriptorPage.GetIncrementSize()) * offset;
	const std::uint64_t gpu =
	    slot.DescriptorPage.GetGPU().ptr + static_cast<std::uint64_t>(slot.DescriptorPage.GetIncrementSize()) * offset;
	return RhiTransientDescriptorRange{
	    .CpuBase = RhiCpuDescriptorHandle{.Value = cpu},
	    .GpuBase = RhiGpuDescriptorHandle{.Value = gpu},
	    .Count = count};
}

void D3D12CommandRecordingContext::ReleaseDescriptorPages() noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	for (auto& frame : m_frames)
	{
		for (QueueFrameState& queue : frame)
		{
			queue.CurrentLease.reset();
			WaitForFrameStateRetirement(queue);

			for (const std::unique_ptr<CommandSlot>& slot : queue.Slots)
			{
				m_descriptorHeapManager->FreeContiguous(
				    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				    slot->DescriptorPage,
				    DescriptorPageCapacity);
			}
		}
	}
}

void D3D12CommandRecordingContext::CloseLease(void* state) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->CloseSlot(slot);
}

void D3D12CommandRecordingContext::BeginLease(void* state) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->BeginSlot(slot);
}

void D3D12CommandRecordingContext::ReleaseLease(void* state, bool) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	slot.Owner->ReleaseSlot(slot);
}

RhiTransientDescriptorRange D3D12CommandRecordingContext::AllocateLeaseDescriptors(
    void* state,
    std::uint32_t count) noexcept
{
	auto& slot = *static_cast<CommandSlot*>(state);
	return slot.Owner->AllocateDescriptors(slot, count);
}

const wchar_t* D3D12CommandRecordingContext::QueueTypeName(ERhiQueueType queueType) noexcept
{
	switch (queueType)
	{
		case ERhiQueueType::Graphics:
			return L"Graphics";
		case ERhiQueueType::Compute:
			return L"Compute";
		case ERhiQueueType::Copy:
			return L"Copy";
		case ERhiQueueType::Count:
		default:
			return L"Unknown";
	}
}

[[noreturn]] void D3D12CommandRecordingContext::FailClose(const CommandSlot& slot, HRESULT result) noexcept
{
	const auto logger = Logging::GetOrCreateLogger("RHI.D3D12.Commands");
	RhiDiagnosticMessage diagnosticMessage;
	while (slot.Owner->m_rhi->TryPopDebugMessage(diagnosticMessage))
	{
		SPDLOG_LOGGER_ERROR(
		    logger,
		    "D3D12 validation while closing {} command list: {}",
		    RhiQueueTypeToString(slot.QueueType),
		    diagnosticMessage.Text);
	}

	Diagnostics::Fail(
	    logger,
	    __FILE__,
	    __LINE__,
	    std::format(
	        "Failed to close {} command list for frame {} context {} (HRESULT=0x{:08X})",
	        RhiQueueTypeToString(slot.QueueType),
	        slot.FrameSlot,
	        slot.ContextIndex,
	        static_cast<std::uint32_t>(result)));
	std::terminate();
}

[[noreturn]] void D3D12CommandRecordingContext::FailExhausted(
    ERhiQueueType queueType,
    std::uint32_t frameIndex) noexcept
{
	const auto logger = Logging::GetOrCreateLogger("RHI.D3D12.Commands");
	Diagnostics::Fail(
	    logger,
	    __FILE__,
	    __LINE__,
	    std::format(
	        "D3D12 command-recording contexts exhausted for {} queue frame {} (limit {}).",
	        RhiQueueTypeToString(queueType),
	        frameIndex,
	        MaximumContextsPerFrameQueue));
	std::terminate();
}
