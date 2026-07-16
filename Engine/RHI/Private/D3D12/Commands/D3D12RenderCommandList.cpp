#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Diagnostics/D3D12PixEvents.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "Interop/RhiInteropService.h"
#include "Validation/RhiContract.h"

#include <array>
#include <string>
#include <vector>

D3D12RenderCommandList::D3D12RenderCommandList(
    D3D12RenderHardwareInterface& owner,
    ID3D12GraphicsCommandList7* commandList,
    ERhiQueueType queueType) noexcept :
    m_owner(&owner), m_commandList(commandList), m_queueType(queueType)
{
}

ERhiBackendApi D3D12RenderCommandList::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

void D3D12RenderCommandList::OnResourceTrackingStarted(RhiResourceHandle resource) noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginResourceTracking(resource);
	}
}

void D3D12RenderCommandList::OnResourceTrackingFinished(
	RhiResourceHandle resource,
	RhiSubmissionToken submissionToken) noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->EndResourceTracking(resource, submissionToken);
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

void D3D12RenderCommandList::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const auto& nativePipelineState = static_cast<const D3D12PipelineState&>(pipelineState);
	m_commandList->SetPipelineState(nativePipelineState.Get().Get());
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

void D3D12RenderCommandList::ResetBoundState() noexcept
{
}

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

void D3D12RenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = D3D12TypeConversions::ToCpuDescriptor(rtv);
	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    dsv != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(numRTVs);
	for (std::uint32_t index = 0; index < numRTVs; ++index)
	{
		nativeRtvs[index] = D3D12TypeConversions::ToCpuDescriptor(rtvs[index]);
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv =
	    dsv != nullptr ? D3D12TypeConversions::ToCpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_commandList->OMSetRenderTargets(numRTVs, nativeRtvs.data(), FALSE, dsv != nullptr ? &nativeDsv : nullptr);
}

void D3D12RenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearRenderTargetView(D3D12TypeConversions::ToCpuDescriptor(rtv), color, 0, nullptr);
	}
}

void D3D12RenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandList != nullptr)
	{
		m_commandList->ClearDepthStencilView(
		    D3D12TypeConversions::ToCpuDescriptor(dsv),
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
	if (m_commandList == nullptr ||
	    !RhiContract::IsRayTracingGeometryDescUsable(geometry) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(scratchGpuAddress) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(resultGpuAddress))
	{
		return;
	}

	D3D12_RAYTRACING_GEOMETRY_DESC nativeGeometry{};
	nativeGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	nativeGeometry.Flags = geometry.Opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
	nativeGeometry.Triangles.Transform3x4 = 0;
	nativeGeometry.Triangles.IndexFormat = D3D12TypeConversions::ToIndexFormat(geometry.IndexFormat);
	nativeGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	nativeGeometry.Triangles.IndexCount = geometry.IndexCount;
	nativeGeometry.Triangles.VertexCount = geometry.VertexCount;
	nativeGeometry.Triangles.IndexBuffer = geometry.IndexBuffer;
	nativeGeometry.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer;
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

void D3D12RenderCommandList::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress instanceDescsGpuAddress,
    std::uint32_t instanceCount,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress,
    ERhiClassicTlasBuildMode buildMode) noexcept
{
	if (m_commandList == nullptr ||
	    !RhiContract::IsRayTracingGpuAddressPresent(instanceDescsGpuAddress) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(scratchGpuAddress) ||
	    !RhiContract::IsRayTracingGpuAddressPresent(resultGpuAddress) ||
	    instanceCount == 0)
	{
		return;
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
	if (m_commandList == nullptr || m_owner == nullptr)
	{
		return;
	}

	BeginDiagnosticScope("RayTracing.PTLAS.Build", RhiDiagnosticLabelColor{92, 148, 255, 255});
	const bool submitted = m_owner->BuildPartitionedTopLevelAccelerationStructure(m_commandList, desc);
	EndDiagnosticScope();
	if (submitted)
	{
		UnorderedAccessBarrier(RhiResourceHandle{nullptr});
	}
}

void D3D12RenderCommandList::CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept
{
	TrackResource(destinationResource);
	TrackResource(sourceResource);
	if (m_commandList != nullptr)
	{
		m_commandList->CopyResource(
		    D3D12TypeConversions::ToResource(destinationResource),
		    D3D12TypeConversions::ToResource(sourceResource));
	}
}

void D3D12RenderCommandList::AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept
{
	if (m_commandList == nullptr)
	{
		return;
	}

	TrackResource(beforeResource);
	TrackResource(afterResource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = nullptr;
	barrier.Aliasing.pResourceAfter = nullptr;
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	TrackResource(resource);
	if (m_commandList == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = D3D12TypeConversions::ToResource(resource);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	const auto resolveState = [this](ResourceState state)
	{
		if (m_queueType == ERhiQueueType::Compute && state == ResourceState::ShaderResource)
		{
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
		return D3D12TypeConversions::ToResourceStates(state);
	};
	barrier.Transition.StateBefore = resolveState(before);
	barrier.Transition.StateAfter = resolveState(after);
	if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
	{
		return;
	}
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::UnorderedAccessBarrier(RhiResourceHandle resource) noexcept
{
	TrackResource(resource);
	if (m_commandList == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = D3D12TypeConversions::ToResource(resource);
	m_commandList->ResourceBarrier(1, &barrier);
}
