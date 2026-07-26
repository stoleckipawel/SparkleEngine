#pragma once

#include "Commands/RhiCommandRecordingLease.h"
#include "Frame/RhiFrameConstants.h"
#include "D3D12/Commands/D3D12RecordingUploadPage.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"

#include <array>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <vector>
#include <wrl/client.h>

class D3D12DescriptorHeapManager;
class D3D12RenderCommandList;
class D3D12RenderHardwareInterface;
class D3D12Rhi;
class RenderCommandList;

class D3D12CommandRecordingContext final
{
  public:
	static constexpr std::uint32_t MaximumContextsPerFrameQueue = 8;
	static constexpr std::uint64_t UploadPageCapacityInBytes = 256 * 1024;
	static constexpr std::uint32_t DescriptorPageCapacity = 256;

	D3D12CommandRecordingContext(
	    D3D12Rhi& rhi,
	    D3D12RenderHardwareInterface& hardwareInterface,
	    D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;
	~D3D12CommandRecordingContext() noexcept;

	D3D12CommandRecordingContext(const D3D12CommandRecordingContext&) = delete;
	D3D12CommandRecordingContext& operator=(const D3D12CommandRecordingContext&) = delete;
	D3D12CommandRecordingContext(D3D12CommandRecordingContext&&) = delete;
	D3D12CommandRecordingContext& operator=(D3D12CommandRecordingContext&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiCommandRecordingLease Acquire(
	    ERhiQueueType queueType,
	    std::uint32_t frameIndex,
	    RhiCommandRecordingOwner owner) noexcept;
	RhiSubmissionToken Submit(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept;
	RhiSubmissionToken SubmitBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept;
	RenderCommandList& BeginCurrentGraphicsCommandList(std::uint32_t frameIndex) noexcept;
	RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease(
	    std::uint32_t frameIndex) noexcept;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(
	    std::uint32_t frameIndex,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept;

	RenderCommandList& GetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RenderCommandList* TryGetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
  private:
	enum class SlotState : std::uint8_t
	{
		Available,
		Recording,
		Closed,
		Submitted,
	};

	struct CommandSlot final
	{
		D3D12CommandRecordingContext* Owner = nullptr;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> NativeCommandList;
		std::unique_ptr<D3D12RenderCommandList> CommandList;
		D3D12RecordingUploadPage UploadPage;
		D3D12DescriptorHandle DescriptorPage;
		RhiSubmissionToken RetirementToken = {};
		RhiCommandRecordingOwner RecordingOwner = {};
		std::thread::id RecordingThread;
		std::uint32_t FrameSlot = 0;
		std::uint32_t ContextIndex = 0;
		std::uint32_t DescriptorOffset = 0;
		ERhiQueueType QueueType = ERhiQueueType::Graphics;
		SlotState State = SlotState::Available;
	};

	struct QueueFrameState final
	{
		std::vector<std::unique_ptr<CommandSlot>> Slots;
		std::optional<RhiCommandRecordingLease> CurrentLease;
	};

	QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	CommandSlot& AcquireSlot(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	void InitializeSlots();
	CommandSlot& CreateSlot(ERhiQueueType queueType, std::uint32_t frameIndex);
	void CreateNativeCommandObjects(CommandSlot& slot);
	void InitializeSlotResources(CommandSlot& slot);
	void NameSlotObjects(CommandSlot& slot) const noexcept;
	void WaitForFrameStateRetirement(
	    const QueueFrameState& frameState) noexcept;
	void ResetSlot(CommandSlot& slot) noexcept;
	void BeginSlot(CommandSlot& slot) noexcept;
	void CloseSlot(CommandSlot& slot) noexcept;
	void ReleaseSlot(CommandSlot& slot) noexcept;
	CommandSlot* ConsumeClosedLease(
	    RhiCommandRecordingLease&& lease) noexcept;
	void ResolveSubmittedSlot(
	    CommandSlot& slot,
	    RhiSubmissionToken token) noexcept;
	RhiTransientDescriptorRange AllocateDescriptors(CommandSlot& slot, std::uint32_t count) noexcept;
	void ReleaseDescriptorPages() noexcept;

	static void BeginLease(void* state) noexcept;
	static void CloseLease(void* state) noexcept;
	static void ReleaseLease(void* state, bool closed) noexcept;
	static RhiTransientDescriptorRange AllocateLeaseDescriptors(void* state, std::uint32_t count) noexcept;
	static const wchar_t* QueueTypeName(ERhiQueueType queueType) noexcept;
	[[noreturn]] static void FailClose(const CommandSlot& slot, HRESULT result) noexcept;
	[[noreturn]] static void FailExhausted(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12RenderHardwareInterface* m_hardwareInterface = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	std::array<std::array<QueueFrameState, RhiQueueTypeCount>, RhiFrameConstants::FramesInFlight> m_frames;
};
