#pragma once

#include "RayTracing/RhiRayTracingPipelineDesc.h"

#include <d3d12.h>
#include <string_view>
#include <wrl/client.h>

class D3D12Rhi;

class D3D12RayTracingPipeline final : public RayTracingPipeline
{
public:
	D3D12RayTracingPipeline(D3D12Rhi& rhi, const RayTracingPipelineDesc& desc);

	ID3D12StateObject* GetStateObject() const noexcept { return m_stateObject.Get(); }
	const void* FindShaderIdentifier(std::string_view exportName) const noexcept;

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_stateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_properties;
};
