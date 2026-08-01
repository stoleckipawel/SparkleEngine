#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Validation/RhiContract.h"

static const auto g_d3d12RenderCommandListLogger = Logging::GetOrCreateLogger("RHI.D3D12.CommandList");

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
