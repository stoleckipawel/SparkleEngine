#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Interop/RhiInteropService.h"
#include "Validation/RhiContract.h"

#include <array>
#include <string>
#include <vector>

static const auto g_d3d12RenderCommandListLogger = Logging::GetOrCreateLogger("RHI.D3D12.CommandList");

D3D12RenderCommandList::D3D12RenderCommandList(
    D3D12RenderHardwareInterface& owner,
    ID3D12GraphicsCommandList7* commandList,
    ERhiQueueType queueType) noexcept :
    m_owner(&owner), m_commandList(commandList), m_queueType(queueType)
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

void D3D12RenderCommandList::ResetBoundState() noexcept {}

void D3D12RenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootConstantBufferView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootShaderResourceView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootUnorderedAccessView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetGraphicsRootDescriptorTable(
	    bindingIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRootDescriptorTable(bindingIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetGraphicsPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetGraphicsRoot32BitConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}

void D3D12RenderCommandList::BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootConstantBufferView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootShaderResourceView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootUnorderedAccessView(bindingIndex, gpuAddress);
	}
}

void D3D12RenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !tableBinding)
	{
		return;
	}

	m_commandList->SetComputeRootDescriptorTable(
	    bindingIndex,
	    m_owner->ResolveDescriptorTableGpuHandle(tableBinding.Table, tableBinding.DescriptorIndex));
}

void D3D12RenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRootDescriptorTable(bindingIndex, D3D12TypeConversions::ToGpuDescriptor(baseDescriptor));
	}
}

void D3D12RenderCommandList::SetComputePushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->SetComputeRoot32BitConstants(bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
	}
}

void D3D12RenderCommandList::SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->IASetPrimitiveTopology(D3D12TypeConversions::ToPrimitiveTopology(topology));
	}
}

void D3D12RenderCommandList::BindVertexBuffer(const RhiVertexBufferView& view) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_VERTEX_BUFFER_VIEW nativeView{
	    .BufferLocation = view.BufferLocation,
	    .SizeInBytes = view.SizeInBytes,
	    .StrideInBytes = view.StrideInBytes};
	m_commandList->IASetVertexBuffers(0, 1, &nativeView);
}

void D3D12RenderCommandList::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_INDEX_BUFFER_VIEW nativeView{
	    .BufferLocation = view.BufferLocation,
	    .SizeInBytes = view.SizeInBytes,
	    .Format = D3D12TypeConversions::ToIndexFormat(view.Format)};
	m_commandList->IASetIndexBuffer(&nativeView);
}

void D3D12RenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle renderTarget, const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = D3D12TypeConversions::ToCpuDescriptor(renderTarget);
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    depthStencil != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*depthStencil) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, depthStencil != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::SetRenderTargets(
    std::uint32_t renderTargetCount,
    const RhiCpuDescriptorHandle* renderTargets,
    const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}
	if (renderTargetCount > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT || (renderTargetCount != 0 && renderTargets == nullptr))
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 SetRenderTargets received an invalid render-target count or array.");
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(renderTargetCount);
	for (std::uint32_t index = 0; index < renderTargetCount; ++index)
	{
		nativeRtvs[index] = D3D12TypeConversions::ToCpuDescriptor(renderTargets[index]);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    depthStencil != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*depthStencil) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(renderTargetCount, nativeRtvs.data(), FALSE, depthStencil != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle renderTarget, const float color[4]) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearRenderTargetView(D3D12TypeConversions::ToCpuDescriptor(renderTarget), color, 0, nullptr);
	}
}

void D3D12RenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle depthStencil, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearDepthStencilView(
		    D3D12TypeConversions::ToCpuDescriptor(depthStencil),
		    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		    depth,
		    stencil,
		    0,
		    nullptr);
	}
}

void D3D12RenderCommandList::SetViewport(const RhiViewport& viewport) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_VIEWPORT nativeViewport{
	    .TopLeftX = viewport.X,
	    .TopLeftY = viewport.Y,
	    .Width = viewport.Width,
	    .Height = viewport.Height,
	    .MinDepth = viewport.MinDepth,
	    .MaxDepth = viewport.MaxDepth};
	m_commandList->RSSetViewports(1, &nativeViewport);
}

void D3D12RenderCommandList::SetScissorRect(const RhiRect& rect) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_RECT nativeRect{.left = rect.Left, .top = rect.Top, .right = rect.Right, .bottom = rect.Bottom};
	m_commandList->RSSetScissorRects(1, &nativeRect);
}

void D3D12RenderCommandList::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList
		    ->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
}

void D3D12RenderCommandList::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}
}

void D3D12RenderCommandList::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}
}

void D3D12RenderCommandList::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc& geometry,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	const D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAddress = ResolveRayTracingBufferAddress(geometry.VertexBuffer);

	const D3D12_GPU_VIRTUAL_ADDRESS indexBufferAddress = ResolveRayTracingBufferAddress(geometry.IndexBuffer);
	if (m_commandList == nullptr || !RhiContract::IsRayTracingGeometryDescUsable(geometry) || vertexBufferAddress == 0 ||
	    indexBufferAddress == 0 || !RhiContract::IsRayTracingGpuAddressPresent(scratchGpuAddress) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(resultGpuAddress))
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 BLAS build received incomplete geometry, command-list, or GPU-address inputs.");
	}

	TrackResource(geometry.VertexBuffer.Resource);
	TrackResource(geometry.IndexBuffer.Resource);

	D3D12_RAYTRACING_GEOMETRY_DESC nativeGeometry{};
	nativeGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	nativeGeometry.Flags = geometry.Opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	nativeGeometry.Triangles.Transform3x4 = 0;
	nativeGeometry.Triangles.IndexFormat = D3D12TypeConversions::ToIndexFormat(geometry.IndexFormat);
	nativeGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	nativeGeometry.Triangles.IndexCount = geometry.IndexCount;
	nativeGeometry.Triangles.VertexCount = geometry.VertexCount;
	nativeGeometry.Triangles.IndexBuffer = indexBufferAddress;
	nativeGeometry.Triangles.VertexBuffer.StartAddress = vertexBufferAddress;
	nativeGeometry.Triangles.VertexBuffer.StrideInBytes = geometry.VertexStrideInBytes;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &nativeGeometry;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = scratchGpuAddress;
	buildDesc.DestAccelerationStructureData = resultGpuAddress;
	m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RenderCommandList::ResolveRayTracingBufferAddress(const RhiRayTracingBufferBinding& binding) noexcept
{
	ID3D12Resource* const resource = static_cast<ID3D12Resource*>(binding.Resource.Value);
	return resource != nullptr ? resource->GetGPUVirtualAddress() + binding.OffsetInBytes : 0;
}

void D3D12RenderCommandList::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress instanceDescsGpuAddress,
    std::uint32_t instanceCount,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress,
    ERhiClassicTlasBuildMode buildMode) noexcept
{
	if (m_commandList == nullptr || !RhiContract::IsRayTracingGpuAddressPresent(instanceDescsGpuAddress) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(scratchGpuAddress) || !RhiContract::IsRayTracingGpuAddressPresent(resultGpuAddress))
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 classic TLAS build received no command list or an empty GPU address.");
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	if (buildMode != ERhiClassicTlasBuildMode::Build)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}
	if (buildMode == ERhiClassicTlasBuildMode::Update)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}
	inputs.NumDescs = instanceCount;
	inputs.InstanceDescs = instanceDescsGpuAddress;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = scratchGpuAddress;
	buildDesc.DestAccelerationStructureData = resultGpuAddress;
	buildDesc.SourceAccelerationStructureData = buildMode == ERhiClassicTlasBuildMode::Update ? resultGpuAddress : 0;
	m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

void D3D12RenderCommandList::BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc& desc) noexcept
{
	if (m_commandList == nullptr || m_owner == nullptr || !desc.DestinationResource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 partitioned TLAS build has no command list, device owner, or destination resource.");
	}

	BeginDiagnosticScope("RayTracing.PTLAS.Build", RhiDiagnosticLabelColor{92, 148, 255, 255});
	const bool submitted = m_owner->BuildPartitionedTopLevelAccelerationStructure(m_commandList, desc);
	EndDiagnosticScope();
	if (!submitted)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 partitioned TLAS provider rejected the build command.");
	}
	UnorderedAccessBarrier(desc.DestinationResource);
}

void D3D12RenderCommandList::CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept
{
	if (m_commandList == nullptr || !destinationResource || !sourceResource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 CopyResource requires an active command list and two valid resources.");
	}
	TrackResource(destinationResource);
	TrackResource(sourceResource);
	m_commandList->CopyResource(D3D12TypeConversions::ToResource(destinationResource), D3D12TypeConversions::ToResource(sourceResource));
}

void D3D12RenderCommandList::AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept
{
	if (m_commandList == nullptr || !beforeResource || !afterResource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 aliasing barriers require an active command list and two valid resources.");
	}

	TrackResource(beforeResource);
	TrackResource(afterResource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = D3D12TypeConversions::ToResource(beforeResource);
	barrier.Aliasing.pResourceAfter = D3D12TypeConversions::ToResource(afterResource);
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandList == nullptr || !resource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 resource transitions require an active command list and a valid resource.");
	}
	if (before == after)
	{
		return;
	}
	TrackResource(resource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = D3D12TypeConversions::ToResource(resource);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ResolveResourceState(before);
	barrier.Transition.StateAfter = ResolveResourceState(after);
	if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
	{
		return;
	}
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::UnorderedAccessBarrier(RhiResourceHandle resource) noexcept
{
	if (m_commandList == nullptr || !resource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 unordered-access barriers require an active command list and a valid resource.");
	}
	TrackResource(resource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = D3D12TypeConversions::ToResource(resource);
	m_commandList->ResourceBarrier(1, &barrier);
}

D3D12_RESOURCE_STATES D3D12RenderCommandList::ResolveResourceState(ResourceState state) const noexcept
{
	if (m_queueType == ERhiQueueType::Compute && state == ResourceState::ShaderResource)
	{
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	}
	return D3D12TypeConversions::ToResourceStates(state);
}
