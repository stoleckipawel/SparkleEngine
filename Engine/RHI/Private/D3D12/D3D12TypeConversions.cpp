#include "PCH.h"

#include "D3D12/D3D12TypeConversions.h"

DXGI_FORMAT D3D12TypeConversions::ToDxgiFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::B8G8R8A8_UNorm:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::D24_UNorm_S8_UInt:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::R32_Float:
			return DXGI_FORMAT_R32_FLOAT;
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