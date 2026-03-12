// =============================================================================
// D3D12VertexLayout.h — Standard vertex input layouts for D3D12 pipelines
// =============================================================================
//
// Provides static vertex layout descriptors matching VertexData struct.
// Used by pipeline state creation.
//
// =============================================================================

#pragma once

#include <d3d12.h>
#include <span>

namespace D3D12VertexLayout
{
	// Standard vertex layout matching VertexData struct:
	//   float3 position  -> POSITION
	//   float2 uv        -> TEXCOORD
	//   float4 color     -> COLOR
	//   float3 normal    -> NORMAL
	//   float4 tangent   -> TANGENT

	inline constexpr D3D12_INPUT_ELEMENT_DESC kStaticMeshLayout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	inline std::span<const D3D12_INPUT_ELEMENT_DESC> GetStaticMeshLayout() noexcept
	{
		return kStaticMeshLayout;
	}

}  // namespace D3D12VertexLayout
