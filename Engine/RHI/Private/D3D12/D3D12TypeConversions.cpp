#include "PCH.h"

#include "D3D12/D3D12TypeConversions.h"

class D3D12TypeConversionsOperations final
{
  public:
	static DXGI_FORMAT ResolveTextureResourceFormat(const RhiTextureResourceDesc& desc) noexcept
	{
		if (!desc.AllowDepthStencil)
		{
			return D3D12TypeConversions::ToDxgiFormat(desc.Format);
		}

		switch (desc.Format)
		{
			case PixelFormat::D32_Float:
				return DXGI_FORMAT_R32_TYPELESS;
			case PixelFormat::D24_UNorm_S8_UInt:
				return DXGI_FORMAT_R24G8_TYPELESS;
			default:
				return D3D12TypeConversions::ToDxgiFormat(desc.Format);
		}
	}
};

DXGI_FORMAT D3D12TypeConversions::ToDxgiFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PixelFormat::R16G16B16A16_Float:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case PixelFormat::R16G16_Float:
			return DXGI_FORMAT_R16G16_FLOAT;
		case PixelFormat::R8G8B8A8_UNorm:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case PixelFormat::B8G8R8A8_UNorm:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case PixelFormat::D32_Float:
			return DXGI_FORMAT_D32_FLOAT;
		case PixelFormat::R32_Float:
			return DXGI_FORMAT_R32_FLOAT;
		case PixelFormat::D24_UNorm_S8_UInt:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::BC1_UNorm:
			return DXGI_FORMAT_BC1_UNORM;
		case PixelFormat::BC1_UNorm_Srgb:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case PixelFormat::BC2_UNorm:
			return DXGI_FORMAT_BC2_UNORM;
		case PixelFormat::BC2_UNorm_Srgb:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case PixelFormat::BC3_UNorm:
			return DXGI_FORMAT_BC3_UNORM;
		case PixelFormat::BC3_UNorm_Srgb:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case PixelFormat::BC4_UNorm:
			return DXGI_FORMAT_BC4_UNORM;
		case PixelFormat::BC4_SNorm:
			return DXGI_FORMAT_BC4_SNORM;
		case PixelFormat::BC5_UNorm:
			return DXGI_FORMAT_BC5_UNORM;
		case PixelFormat::BC5_SNorm:
			return DXGI_FORMAT_BC5_SNORM;
		case PixelFormat::BC6H_UF16:
			return DXGI_FORMAT_BC6H_UF16;
		case PixelFormat::BC7_UNorm:
			return DXGI_FORMAT_BC7_UNORM;
		case PixelFormat::BC7_UNorm_Srgb:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		case PixelFormat::Unknown:
		default:
			return DXGI_FORMAT_UNKNOWN;
	}
}

D3D12_COMPARISON_FUNC D3D12TypeConversions::ToComparisonFunc(CompareOp compareOp) noexcept
{
	switch (compareOp)
	{
		case CompareOp::Never:
			return D3D12_COMPARISON_FUNC_NEVER;
		case CompareOp::Less:
			return D3D12_COMPARISON_FUNC_LESS;
		case CompareOp::Equal:
			return D3D12_COMPARISON_FUNC_EQUAL;
		case CompareOp::LessOrEqual:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case CompareOp::Greater:
			return D3D12_COMPARISON_FUNC_GREATER;
		case CompareOp::NotEqual:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case CompareOp::GreaterOrEqual:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case CompareOp::Always:
		default:
			return D3D12_COMPARISON_FUNC_ALWAYS;
	}
}

ID3D12GraphicsCommandList* D3D12TypeConversions::ToGraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept
{
	return static_cast<ID3D12GraphicsCommandList*>(handle.Value);
}

ID3D12Resource* D3D12TypeConversions::ToResource(RhiResourceHandle handle) noexcept
{
	return static_cast<ID3D12Resource*>(handle.Value);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12TypeConversions::ToCpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
{
	return D3D12_CPU_DESCRIPTOR_HANDLE{handle.Value};
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12TypeConversions::ToGpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
{
	return D3D12_GPU_DESCRIPTOR_HANDLE{handle.Value};
}

D3D12_DESCRIPTOR_HEAP_TYPE D3D12TypeConversions::ToDescriptorHeapType(ERhiDescriptorAllocatorType descriptorType) noexcept
{
	switch (descriptorType)
	{
		case ERhiDescriptorAllocatorType::RenderTarget:
			return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case ERhiDescriptorAllocatorType::DepthStencil:
			return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		case ERhiDescriptorAllocatorType::Sampler:
			return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		case ERhiDescriptorAllocatorType::ShaderResource:
		default:
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	}
}

D3D12_RESOURCE_STATES D3D12TypeConversions::ToResourceStates(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Undefined:
		case ResourceState::Common:
			return D3D12_RESOURCE_STATE_COMMON;
		case ResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case ResourceState::DepthRead:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case ResourceState::ShaderResource:
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case ResourceState::UnorderedAccess:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case ResourceState::RayTracingAccelerationStructure:
			return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		case ResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case ResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case ResourceState::Present:
			return D3D12_RESOURCE_STATE_PRESENT;
		default:
			return D3D12_RESOURCE_STATE_COMMON;
	}
}

D3D12_PRIMITIVE_TOPOLOGY D3D12TypeConversions::ToPrimitiveTopology(RhiPrimitiveTopology topology) noexcept
{
	switch (topology)
	{
		case RhiPrimitiveTopology::TriangleList:
		default:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

DXGI_FORMAT D3D12TypeConversions::ToIndexFormat(RhiIndexFormat format) noexcept
{
	switch (format)
	{
		case RhiIndexFormat::UInt16:
			return DXGI_FORMAT_R16_UINT;
		case RhiIndexFormat::UInt32:
		default:
			return DXGI_FORMAT_R32_UINT;
	}
}

D3D12_RESOURCE_DESC D3D12TypeConversions::BuildTextureResourceDesc(const RhiTextureResourceDesc& desc) noexcept
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = static_cast<UINT64>(desc.Width);
	resourceDesc.Height = desc.Height;
	resourceDesc.DepthOrArraySize = desc.ArraySize;
	resourceDesc.MipLevels = desc.MipLevels;
	resourceDesc.Format = D3D12TypeConversionsOperations::ResolveTextureResourceFormat(desc);
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	if (desc.AllowRenderTarget)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (desc.AllowDepthStencil)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if (desc.AllowUnorderedAccess)
	{
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	return resourceDesc;
}

D3D12_RESOURCE_DESC D3D12TypeConversions::BuildBufferResourceDesc(const RhiBufferResourceDesc& desc) noexcept
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = desc.SizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = desc.AllowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	return resourceDesc;
}

D3D12_CLEAR_VALUE D3D12TypeConversions::BuildClearValue(const RhiOptimizedClearValue& clearValue) noexcept
{
	D3D12_CLEAR_VALUE nativeClearValue{};
	nativeClearValue.Format = ToDxgiFormat(clearValue.Format);
	if (clearValue.ValueType == RhiOptimizedClearValue::Type::DepthStencil)
	{
		nativeClearValue.DepthStencil.Depth = clearValue.Depth;
		nativeClearValue.DepthStencil.Stencil = clearValue.Stencil;
	}
	else
	{
		for (std::size_t index = 0; index < clearValue.Color.size(); ++index)
		{
			nativeClearValue.Color[index] = clearValue.Color[index];
		}
	}
	return nativeClearValue;
}
