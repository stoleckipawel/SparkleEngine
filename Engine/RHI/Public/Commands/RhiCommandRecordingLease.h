#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../RHIAPI.h"
#include "RhiQueue.h"

#include <cstdint>

class RenderCommandList;
class RhiCommandRecordingLeaseAccess;

struct RhiCommandRecordingOwner final
{
	static constexpr std::uint32_t CoordinatorPartitionIndex = ~0u;

	std::uint32_t PartitionIndex = CoordinatorPartitionIndex;
	std::uint64_t TaskIdentity = 0;

	constexpr bool IsCoordinator() const noexcept { return PartitionIndex == CoordinatorPartitionIndex; }
};

struct RhiCommandRecordingContextId final
{
	std::uint32_t Value = ~0u;

	constexpr bool IsValid() const noexcept { return Value != ~0u; }
};

struct RhiCommandRecordingDescriptorPage final
{
	RhiCpuDescriptorHandle CpuBase = {};
	RhiGpuDescriptorHandle GpuBase = {};
	std::uint32_t Capacity = 0;
};

struct RhiTransientDescriptorRange final
{
	RhiCpuDescriptorHandle CpuBase = {};
	RhiGpuDescriptorHandle GpuBase = {};
	std::uint32_t Count = 0;

	constexpr bool IsValid() const noexcept { return static_cast<bool>(CpuBase) && Count != 0; }
};

class SPARKLE_RHI_API RhiCommandRecordingLease final
{
  public:
	RhiCommandRecordingLease() noexcept;
	~RhiCommandRecordingLease() noexcept;

	RhiCommandRecordingLease(const RhiCommandRecordingLease&) = delete;
	RhiCommandRecordingLease& operator=(const RhiCommandRecordingLease&) = delete;
	RhiCommandRecordingLease(RhiCommandRecordingLease&& other) noexcept;
	RhiCommandRecordingLease& operator=(RhiCommandRecordingLease&& other) noexcept;

	ERhiQueueType GetQueueType() const noexcept { return m_queueType; }
	std::uint32_t GetFrameSlot() const noexcept { return m_frameSlot; }
	RhiCommandRecordingContextId GetContextId() const noexcept { return m_contextId; }
	RhiCommandRecordingOwner GetOwner() const noexcept { return m_owner; }
	const RhiCommandRecordingDescriptorPage& GetDescriptorPage() const noexcept { return m_descriptorPage; }
	RhiSubmissionToken GetRetirementToken() const noexcept { return m_retirementToken; }
	bool IsValid() const noexcept { return m_commandList != nullptr; }
	bool IsClosed() const noexcept { return m_closed; }

	RenderCommandList& GetCommandList() noexcept;
	RhiTransientDescriptorRange AllocateTransientDescriptors(std::uint32_t count) noexcept;
	void Close() noexcept;

  private:
	friend class RhiCommandRecordingLeaseAccess;

	using BeginFunction = void (*)(void*) noexcept;
	using CloseFunction = void (*)(void*) noexcept;
	using ReleaseFunction = void (*)(void*, bool) noexcept;
	using AllocateDescriptorsFunction = RhiTransientDescriptorRange (*)(void*, std::uint32_t) noexcept;

	void Reset() noexcept;
	void Release() noexcept;
	void MoveFrom(RhiCommandRecordingLease&& other) noexcept;
	void BeginRecording() noexcept;

	void* m_backendState = nullptr;
	RenderCommandList* m_commandList = nullptr;
	BeginFunction m_begin = nullptr;
	CloseFunction m_close = nullptr;
	ReleaseFunction m_release = nullptr;
	AllocateDescriptorsFunction m_allocateDescriptors = nullptr;
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	std::uint32_t m_frameSlot = 0;
	RhiCommandRecordingContextId m_contextId = {};
	RhiCommandRecordingOwner m_owner = {};
	RhiCommandRecordingDescriptorPage m_descriptorPage = {};
	RhiSubmissionToken m_retirementToken = {};
	bool m_closed = false;
};
