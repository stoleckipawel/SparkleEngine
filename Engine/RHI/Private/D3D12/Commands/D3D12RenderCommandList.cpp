#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Interop/RhiInteropService.h"

#include <algorithm>
#include <array>
#include <string>

static const auto g_d3d12RenderCommandListLogger = Logging::GetOrCreateLogger("RHI.D3D12.CommandList");

D3D12RenderCommandList::D3D12RenderCommandList(
    D3D12RenderHardwareInterface& owner,
    ID3D12GraphicsCommandList7* commandList,
    ERhiQueueType queueType) noexcept :
    m_owner(&owner),
    m_commandList(commandList),
    m_queueType(queueType)
{
	m_recordingResourceUses.reserve(32);
}

ERhiBackendApi D3D12RenderCommandList::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

void D3D12RenderCommandList::OnResourceTrackingStarted(RhiResourceHandle resource) noexcept
{
	if (m_owner != nullptr)
	{
		const D3D12RecordingResourceUseToken use = m_owner->BeginResourceTracking(resource, m_recordingOwner.IsCoordinator());
		m_recordingResourceUses.push_back(RecordingResourceUse{.Resource = resource, .Token = use});
	}
}

void D3D12RenderCommandList::OnResourceTrackingFinished(RhiResourceHandle resource, RhiSubmissionToken submissionToken) noexcept
{
	if (m_owner != nullptr)
	{
		if (m_recordingResourceReleaseIndex >= m_recordingResourceUses.size())
		{
			Diagnostics::Fatal(
			    g_d3d12RenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "D3D12 command-list resource tracking finished without a matching retained resource.");
		}
		const RecordingResourceUse& use = m_recordingResourceUses[m_recordingResourceReleaseIndex++];
		if (use.Resource.Value != resource.Value)
		{
			Diagnostics::Fatal(
			    g_d3d12RenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "D3D12 command-list resource tracking release order does not match its recording order.");
		}
		m_owner->EndResourceTracking(use.Token, submissionToken);

		if (m_recordingResourceReleaseIndex == m_recordingResourceUses.size())
		{
			m_recordingResourceUses.clear();
			m_recordingResourceReleaseIndex = 0;
		}
	}
}

NativeGraphicsCommandListHandle D3D12RenderCommandList::GetNativeHandle(const RhiNativeInteropRequest& request) const noexcept
{
	return IsRhiNativeInteropRequestValid(request) ? NativeGraphicsCommandListHandle{m_commandList} : NativeGraphicsCommandListHandle{};
}

bool D3D12RenderCommandList::SupportsDiagnosticScopes() const noexcept
{
	return m_commandList != nullptr && D3D12PixEvents::IsAvailable();
}

void D3D12RenderCommandList::BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsDiagnosticScopes() || label.empty())
	{
		return;
	}

	const std::string ownedLabel(label);
	D3D12PixEvents::BeginEvent(m_commandList, D3D12PixEvents::ToColor(color), ownedLabel.c_str());
}

void D3D12RenderCommandList::EndDiagnosticScope() noexcept
{
	if (SupportsDiagnosticScopes())
	{
		D3D12PixEvents::EndEvent(m_commandList);
	}
}

void D3D12RenderCommandList::InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsDiagnosticScopes() || label.empty())
	{
		return;
	}

	const std::string ownedLabel(label);
	D3D12PixEvents::SetMarker(m_commandList, D3D12PixEvents::ToColor(color), ownedLabel.c_str());
}

void D3D12RenderCommandList::SetShaderVisibleDescriptorHeaps(std::uint32_t heapCount, ID3D12DescriptorHeap* const* heaps) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	std::array<ID3D12DescriptorHeap*, 2> nativeHeaps{};
	const std::uint32_t clampedHeapCount = std::min<std::uint32_t>(heapCount, static_cast<std::uint32_t>(nativeHeaps.size()));
	for (std::uint32_t index = 0; index < clampedHeapCount; ++index)
	{
		nativeHeaps[index] = heaps[index];
	}

	m_commandList->SetDescriptorHeaps(clampedHeapCount, nativeHeaps.data());
}

void D3D12RenderCommandList::SetPipeline(const RenderPipeline& pipeline) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& d3d12Pipeline = static_cast<const D3D12Pipeline&>(pipeline);
	m_commandList->SetPipelineState(d3d12Pipeline.Get().Get());
}

void D3D12RenderCommandList::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
	m_commandList->SetGraphicsRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
}

void D3D12RenderCommandList::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
	m_commandList->SetComputeRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
}

void D3D12RenderCommandList::SetRayTracingBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}
	const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
	m_commandList->SetComputeRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
}

void D3D12RenderCommandList::ResetBoundState() noexcept
{
	m_boundRayTracingPipeline = nullptr;
}
