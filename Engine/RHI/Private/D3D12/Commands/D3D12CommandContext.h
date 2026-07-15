#pragma once

#include "Commands/RhiQueue.h"
#include "Frame/RhiFrameConstants.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class D3D12RenderCommandList;
class D3D12RenderHardwareInterface;
class D3D12Rhi;
class RenderCommandList;

class D3D12CommandContext final
{
  public:
	D3D12CommandContext(D3D12Rhi& rhi, D3D12RenderHardwareInterface& hardwareInterface) noexcept;
	~D3D12CommandContext() noexcept;

	D3D12CommandContext(const D3D12CommandContext&) = delete;
	D3D12CommandContext& operator=(const D3D12CommandContext&) = delete;
	D3D12CommandContext(D3D12CommandContext&&) = delete;
	D3D12CommandContext& operator=(D3D12CommandContext&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RenderCommandList& BeginCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::uint32_t frameIndex,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept;
	void CancelFrame(std::uint32_t frameIndex) noexcept;

	RenderCommandList& GetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RenderCommandList* TryGetCurrentCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	bool IsRecording(const RenderCommandList& commandList, std::uint32_t frameIndex) const noexcept;

  private:
	struct CommandSlot final
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> NativeCommandList;
		std::unique_ptr<D3D12RenderCommandList> CommandList;
		bool Recording = false;
	};

	struct QueueFrameState final
	{
		std::vector<std::unique_ptr<CommandSlot>> Slots;
		std::size_t NextSlot = 0;
		CommandSlot* CurrentSlot = nullptr;
		RhiSubmissionToken LastSubmission{};
	};

	QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	const QueueFrameState& GetQueueFrameState(ERhiQueueType queueType, std::uint32_t frameIndex) const noexcept;
	CommandSlot& GetOrCreateSlot(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	CommandSlot* FindSlot(RenderCommandList& commandList, std::uint32_t frameIndex) noexcept;
	const CommandSlot* FindSlot(const RenderCommandList& commandList, std::uint32_t frameIndex) const noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12RenderHardwareInterface* m_hardwareInterface = nullptr;
	std::array<std::array<QueueFrameState, RhiQueueTypeCount>, RhiFrameConstants::FramesInFlight> m_frames;
};
