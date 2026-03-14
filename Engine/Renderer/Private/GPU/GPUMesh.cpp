#include "PCH.h"
#include "Renderer/Public/GPU/GPUMesh.h"

#include "D3D12Rhi.h"
#include "Scene/MeshData.h"
#include "Log.h"

#include <d3dx12.h>
#include <cstring>

bool GPUMesh::Upload(D3D12Rhi& rhi, const MeshData& meshData)
{
	if (!meshData.IsValid())
	{
		LOG_ERROR("[GPUMesh] Cannot upload invalid MeshData (empty vertices or indices)");
		return false;
	}

	const auto vertexBufferSize = static_cast<UINT64>(meshData.GetVertexBufferSize());
	const auto indexBufferSize = static_cast<UINT64>(meshData.GetIndexBufferSize());

	D3D12_RESOURCE_DESC vertexDesc{};
	vertexDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	vertexDesc.Width = vertexBufferSize;
	vertexDesc.Height = 1;
	vertexDesc.DepthOrArraySize = 1;
	vertexDesc.MipLevels = 1;
	vertexDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexDesc.SampleDesc.Count = 1;
	vertexDesc.SampleDesc.Quality = 0;
	vertexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	HRESULT hr = rhi.GetDevice()->CreateCommittedResource(
	    &heapProps,
	    D3D12_HEAP_FLAG_NONE,
	    &vertexDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(m_vertexBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR("[GPUMesh] Failed to create vertex buffer");
		return false;
	}

	m_vertexBuffer->SetName(L"GPUMesh_VertexBuffer");

	void* mappedVertex = nullptr;
	D3D12_RANGE readRange{0, 0};
	hr = m_vertexBuffer->Map(0, &readRange, &mappedVertex);
	if (FAILED(hr))
	{
		LOG_ERROR("[GPUMesh] Failed to map vertex buffer");
		return false;
	}

	std::memcpy(mappedVertex, meshData.GetVertexData(), meshData.GetVertexBufferSize());
	m_vertexBuffer->Unmap(0, nullptr);

	D3D12_RESOURCE_DESC indexDesc = vertexDesc;
	indexDesc.Width = indexBufferSize;

	hr = rhi.GetDevice()->CreateCommittedResource(
	    &heapProps,
	    D3D12_HEAP_FLAG_NONE,
	    &indexDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(m_indexBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR("[GPUMesh] Failed to create index buffer");
		m_vertexBuffer.Reset();
		return false;
	}

	m_indexBuffer->SetName(L"GPUMesh_IndexBuffer");

	void* mappedIndex = nullptr;
	hr = m_indexBuffer->Map(0, &readRange, &mappedIndex);
	if (FAILED(hr))
	{
		LOG_ERROR("[GPUMesh] Failed to map index buffer");
		m_vertexBuffer.Reset();
		m_indexBuffer.Reset();
		return false;
	}

	std::memcpy(mappedIndex, meshData.GetIndexData(), meshData.GetIndexBufferSize());
	m_indexBuffer->Unmap(0, nullptr);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	m_vertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = static_cast<UINT>(indexBufferSize);
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	m_vertexCount = meshData.GetVertexCount();
	m_indexCount = meshData.GetIndexCount();

	LOG_TRACE("[GPUMesh] Uploaded mesh buffers");

	return true;
}

void GPUMesh::Bind(ID3D12GraphicsCommandList* cmdList) const noexcept
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->IASetIndexBuffer(&m_indexBufferView);
}
