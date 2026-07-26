#include "PCH.h"

#include "Commands/RhiCommandRecordingLease.h"
#include "Commands/RhiCommandRecordingLeaseAccess.h"
#include "Commands/RenderCommandList.h"

#include <cassert>
#include <type_traits>
#include <utility>

static_assert(!std::is_copy_constructible_v<RhiCommandRecordingLease>);
static_assert(!std::is_copy_assignable_v<RhiCommandRecordingLease>);
static_assert(std::is_move_constructible_v<RhiCommandRecordingLease>);
static_assert(std::is_move_assignable_v<RhiCommandRecordingLease>);
static_assert(!std::is_convertible_v<RhiCommandRecordingLease, RhiSubmissionToken>);
static_assert(!std::is_convertible_v<RhiCommandRecordingLease, RenderCommandList&>);

RhiCommandRecordingLease::RhiCommandRecordingLease() noexcept = default;

RhiCommandRecordingLease::~RhiCommandRecordingLease() noexcept
{
	Release();
}

RhiCommandRecordingLease::RhiCommandRecordingLease(RhiCommandRecordingLease&& other) noexcept
{
	MoveFrom(std::move(other));
}

RhiCommandRecordingLease& RhiCommandRecordingLease::operator=(RhiCommandRecordingLease&& other) noexcept
{
	if (this != &other)
	{
		Release();
		MoveFrom(std::move(other));
	}

	return *this;
}

RenderCommandList& RhiCommandRecordingLease::GetCommandList() noexcept
{
	assert(m_commandList != nullptr);
	assert(!m_closed);
	BeginRecording();
	return *m_commandList;
}

RhiTransientDescriptorRange RhiCommandRecordingLease::AllocateTransientDescriptors(std::uint32_t count) noexcept
{
	if (m_backendState == nullptr || m_allocateDescriptors == nullptr || m_closed || count == 0)
	{
		return {};
	}

	BeginRecording();
	return m_allocateDescriptors(m_backendState, count);
}

void RhiCommandRecordingLease::Close() noexcept
{
	if (m_backendState == nullptr || m_closed)
	{
		return;
	}

	BeginRecording();
	if (m_close != nullptr)
	{
		m_close(m_backendState);
	}
	m_closed = true;
}

void RhiCommandRecordingLease::Reset() noexcept
{
	m_backendState = nullptr;
	m_commandList = nullptr;
	m_begin = nullptr;
	m_close = nullptr;
	m_release = nullptr;
	m_allocateDescriptors = nullptr;
	m_queueType = ERhiQueueType::Graphics;
	m_frameSlot = 0;
	m_contextId = {};
	m_owner = {};
	m_uploadPage = {};
	m_descriptorPage = {};
	m_retirementToken = {};
	m_closed = false;
}

void RhiCommandRecordingLease::Release() noexcept
{
	if (m_backendState != nullptr && m_release != nullptr)
	{
		m_release(m_backendState, m_closed);
	}
	Reset();
}

void RhiCommandRecordingLease::MoveFrom(RhiCommandRecordingLease&& other) noexcept
{
	m_backendState = other.m_backendState;
	m_commandList = other.m_commandList;
	m_begin = other.m_begin;
	m_close = other.m_close;
	m_release = other.m_release;
	m_allocateDescriptors = other.m_allocateDescriptors;
	m_queueType = other.m_queueType;
	m_frameSlot = other.m_frameSlot;
	m_contextId = other.m_contextId;
	m_owner = other.m_owner;
	m_uploadPage = other.m_uploadPage;
	m_descriptorPage = other.m_descriptorPage;
	m_retirementToken = other.m_retirementToken;
	m_closed = other.m_closed;

	other.Reset();
}

void RhiCommandRecordingLease::BeginRecording() noexcept
{
	if (m_begin != nullptr)
	{
		m_begin(m_backendState);
	}
}

RhiCommandRecordingLease RhiCommandRecordingLeaseAccess::Create(
    const RhiCommandRecordingLeaseInitialization& initialization) noexcept
{
	RhiCommandRecordingLease lease;
	lease.m_backendState = initialization.BackendState;
	lease.m_commandList = initialization.CommandList;
	lease.m_begin = initialization.Begin;
	lease.m_close = initialization.Close;
	lease.m_release = initialization.Release;
	lease.m_allocateDescriptors = initialization.AllocateDescriptors;
	lease.m_queueType = initialization.QueueType;
	lease.m_frameSlot = initialization.FrameSlot;
	lease.m_contextId = initialization.ContextId;
	lease.m_owner = initialization.Owner;
	lease.m_uploadPage = initialization.UploadPage;
	lease.m_descriptorPage = initialization.DescriptorPage;
	lease.m_retirementToken = initialization.RetirementToken;
	return lease;
}

RhiCommandRecordingLeaseBackendState RhiCommandRecordingLeaseAccess::Consume(
    RhiCommandRecordingLease&& lease) noexcept
{
	RhiCommandRecordingLeaseBackendState state{
	    .State = lease.m_backendState,
	    .CommandList = lease.m_commandList,
	    .QueueType = lease.m_queueType,
	    .FrameSlot = lease.m_frameSlot,
	    .ContextId = lease.m_contextId,
	    .Owner = lease.m_owner,
	    .Closed = lease.m_closed};
	lease.Reset();
	return state;
}
