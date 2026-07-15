#include "PCH.h"

#include "D3D12/Commands/D3D12CommandContext.h"

#include "D3D12/Commands/D3D12CommandQueue.h"
#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Device/D3D12Rhi.h"

#include <format>

namespace
{
	static const auto g_d3d12CommandContextLogger = Logging::GetOrCreateLogger("RHI.D3D12.Commands");

	const wchar_t* QueueTypeToWideString(ERhiQueueType queueType) noexcept
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
}

D3D12CommandContext::D3D12CommandContext(
	D3D12Rhi& rhi,
	D3D12RenderHardwareInterface& hardwareInterface) noexcept :
	m_rhi(&rhi), m_hardwareInterface(&hardwareInterface)
{
}

D3D12CommandContext::~D3D12CommandContext() noexcept = default;

void D3D12CommandContext::BeginFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		m_rhi->WaitForSubmission(frameState.LastSubmission);
		frameState.NextSlot = 0;
		frameState.CurrentSlot = nullptr;
		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			slot->Recording = false;
		}
	}
}

RenderCommandList& D3D12CommandContext::BeginCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	CommandSlot& slot = GetOrCreateSlot(queueType, frameIndex);
	CHECK(slot.Allocator->Reset());
	CHECK(slot.NativeCommandList->Reset(slot.Allocator.Get(), nullptr));
	slot.CommandList->ResetTrackedResources();
	slot.CommandList->ResetBoundState();
	slot.Recording = true;
	GetQueueFrameState(queueType, frameIndex).CurrentSlot = &slot;
	return *slot.CommandList;
}

RhiSubmissionToken D3D12CommandContext::SubmitCommandList(
	RenderCommandList& commandList,
	std::uint32_t frameIndex,
	std::span<const RhiSubmissionToken> waitTokens) noexcept
{
	CommandSlot* slot = FindSlot(commandList, frameIndex);
	if (slot == nullptr || !slot->Recording)
	{
		Diagnostics::Fail(
		    g_d3d12CommandContextLogger,
		    __FILE__,
		    __LINE__,
		    "SubmitCommandList requires a recording command list from the current frame");
		return {};
	}

	const ERhiQueueType queueType = commandList.GetQueueType();
	for (const RhiSubmissionToken waitToken : waitTokens)
	{
		m_rhi->QueueWait(queueType, waitToken);
	}

	const HRESULT closeResult = slot->NativeCommandList->Close();
	if (FAILED(closeResult))
	{
		RhiDiagnosticMessage diagnosticMessage;
		while (m_rhi->TryPopDebugMessage(diagnosticMessage))
		{
			SPDLOG_LOGGER_ERROR(
			    g_d3d12CommandContextLogger,
			    "D3D12 validation while closing {} command list: {}",
			    RhiQueueTypeToString(commandList.GetQueueType()),
			    diagnosticMessage.Text);
		}
		Diagnostics::Fail(
		    g_d3d12CommandContextLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Failed to close {} command list for frame {} (HRESULT=0x{:08X})",
		        RhiQueueTypeToString(commandList.GetQueueType()),
		        frameIndex,
		        static_cast<std::uint32_t>(closeResult)));
		return {};
	}
	ID3D12CommandList* nativeCommandLists[] = {slot->NativeCommandList.Get()};
	m_rhi->ExecuteCommandLists(queueType, nativeCommandLists);
	const RhiSubmissionToken submissionToken = m_rhi->Signal(queueType);
	commandList.ResolveTrackedResources(submissionToken);
	slot->Recording = false;
	GetQueueFrameState(queueType, frameIndex).LastSubmission = submissionToken;
	return submissionToken;
}

void D3D12CommandContext::CancelFrame(std::uint32_t frameIndex) noexcept
{
	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		QueueFrameState& frameState = GetQueueFrameState(static_cast<ERhiQueueType>(queueIndex), frameIndex);
		for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
		{
			if (slot->Recording)
			{
				(void)SubmitCommandList(*slot->CommandList, frameIndex);
			}
		}
	}
}

RenderCommandList& D3D12CommandContext::GetCurrentCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	RenderCommandList* const commandList = TryGetCurrentCommandList(queueType, frameIndex);
	if (commandList == nullptr)
	{
		Diagnostics::Fail(
		    g_d3d12CommandContextLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "GetCurrentCommandList called before BeginCommandList (queue={}, frame={})",
		        RhiQueueTypeToString(queueType),
		        frameIndex));
	}
	return *commandList;
}

RenderCommandList* D3D12CommandContext::TryGetCurrentCommandList(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	return frameState.CurrentSlot != nullptr ? frameState.CurrentSlot->CommandList.get() : nullptr;
}

bool D3D12CommandContext::IsRecording(
	const RenderCommandList& commandList,
	std::uint32_t frameIndex) const noexcept
{
	const CommandSlot* slot = FindSlot(commandList, frameIndex);
	return slot != nullptr && slot->Recording;
}

D3D12CommandContext::QueueFrameState& D3D12CommandContext::GetQueueFrameState(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	return m_frames[frameIndex % m_frames.size()][RhiQueueTypeToIndex(queueType)];
}

const D3D12CommandContext::QueueFrameState& D3D12CommandContext::GetQueueFrameState(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) const noexcept
{
	return m_frames[frameIndex % m_frames.size()][RhiQueueTypeToIndex(queueType)];
}

D3D12CommandContext::CommandSlot& D3D12CommandContext::GetOrCreateSlot(
	ERhiQueueType queueType,
	std::uint32_t frameIndex) noexcept
{
	QueueFrameState& frameState = GetQueueFrameState(queueType, frameIndex);
	const std::size_t slotIndex = frameState.NextSlot++;
	if (slotIndex < frameState.Slots.size())
	{
		return *frameState.Slots[slotIndex];
	}

	auto slot = std::make_unique<CommandSlot>();
	const D3D12_COMMAND_LIST_TYPE nativeType = D3D12CommandQueue::GetNativeCommandListType(queueType);
	CHECK(m_rhi->GetDevice()->CreateCommandAllocator(
	    nativeType,
	    IID_PPV_ARGS(slot->Allocator.ReleaseAndGetAddressOf())));
	CHECK(m_rhi->GetDevice()->CreateCommandList(
	    0,
	    nativeType,
	    slot->Allocator.Get(),
	    nullptr,
	    IID_PPV_ARGS(slot->NativeCommandList.ReleaseAndGetAddressOf())));
	CHECK(slot->NativeCommandList->Close());
	slot->CommandList = std::make_unique<D3D12RenderCommandList>(
	    *m_hardwareInterface,
	    slot->NativeCommandList.Get(),
	    queueType);

	const std::wstring allocatorName = std::format(
	    L"Sparkle {} Command Allocator Frame {} Slot {}",
	    QueueTypeToWideString(queueType),
	    frameIndex,
	    slotIndex);
	const std::wstring commandListName = std::format(
	    L"Sparkle {} Command List Frame {} Slot {}",
	    QueueTypeToWideString(queueType),
	    frameIndex,
	    slotIndex);
	(void)slot->Allocator->SetName(allocatorName.c_str());
	(void)slot->NativeCommandList->SetName(commandListName.c_str());

	frameState.Slots.push_back(std::move(slot));
	return *frameState.Slots.back();
}

D3D12CommandContext::CommandSlot* D3D12CommandContext::FindSlot(
	RenderCommandList& commandList,
	std::uint32_t frameIndex) noexcept
{
	return const_cast<CommandSlot*>(std::as_const(*this).FindSlot(commandList, frameIndex));
}

const D3D12CommandContext::CommandSlot* D3D12CommandContext::FindSlot(
	const RenderCommandList& commandList,
	std::uint32_t frameIndex) const noexcept
{
	const QueueFrameState& frameState = GetQueueFrameState(commandList.GetQueueType(), frameIndex);
	for (const std::unique_ptr<CommandSlot>& slot : frameState.Slots)
	{
		if (slot->CommandList.get() == &commandList)
		{
			return slot.get();
		}
	}
	return nullptr;
}
