#pragma once

#include <cstdint>
#include <d3d12.h>

namespace RootBindings
{
	namespace RootParam
	{
		constexpr uint32_t PerFrame = 0;
		constexpr uint32_t PerView = 1;
		constexpr uint32_t PerObjectVS = 2;
		constexpr uint32_t PerObjectPS = 3;
		constexpr uint32_t TextureSRV = 4;
		constexpr uint32_t SamplerTable = 5;

		constexpr uint32_t Count = 6;
	}

	namespace CBRegister
	{
		constexpr uint32_t PerFrame = 0;
		constexpr uint32_t PerView = 1;
		constexpr uint32_t PerObjectVS = 2;
		constexpr uint32_t PerObjectPS = 3;
	}

	namespace SRVRegister
	{
		constexpr uint32_t BaseColor = 0;
		constexpr uint32_t Normal = 1;
		constexpr uint32_t MetallicRoughness = 2;
		constexpr uint32_t Occlusion = 3;
		constexpr uint32_t Emissive = 4;

		constexpr uint32_t MaterialTableBase = BaseColor;
		constexpr uint32_t MaterialTextureCount = 5;
	}

	namespace SamplerRegister
	{
		constexpr uint32_t PointMipPointWrap = 0;
		constexpr uint32_t PointMipPointClamp = 1;
		constexpr uint32_t PointMipPointMirror = 2;
		constexpr uint32_t PointMipLinearWrap = 3;
		constexpr uint32_t PointMipLinearClamp = 4;
		constexpr uint32_t PointMipLinearMirror = 5;
		constexpr uint32_t PointNoMipWrap = 6;
		constexpr uint32_t PointNoMipClamp = 7;
		constexpr uint32_t PointNoMipMirror = 8;

		constexpr uint32_t LinearMipPointWrap = 9;
		constexpr uint32_t LinearMipPointClamp = 10;
		constexpr uint32_t LinearMipPointMirror = 11;
		constexpr uint32_t LinearMipLinearWrap = 12;
		constexpr uint32_t LinearMipLinearClamp = 13;
		constexpr uint32_t LinearMipLinearMirror = 14;
		constexpr uint32_t LinearNoMipWrap = 15;
		constexpr uint32_t LinearNoMipClamp = 16;
		constexpr uint32_t LinearNoMipMirror = 17;

		constexpr uint32_t Aniso1xWrap = 18;
		constexpr uint32_t Aniso1xClamp = 19;
		constexpr uint32_t Aniso1xMirror = 20;
		constexpr uint32_t Aniso2xWrap = 21;
		constexpr uint32_t Aniso2xClamp = 22;
		constexpr uint32_t Aniso2xMirror = 23;
		constexpr uint32_t Aniso4xWrap = 24;
		constexpr uint32_t Aniso4xClamp = 25;
		constexpr uint32_t Aniso4xMirror = 26;
		constexpr uint32_t Aniso8xWrap = 27;
		constexpr uint32_t Aniso8xClamp = 28;
		constexpr uint32_t Aniso8xMirror = 29;
		constexpr uint32_t Aniso16xWrap = 30;
		constexpr uint32_t Aniso16xClamp = 31;
		constexpr uint32_t Aniso16xMirror = 32;

		constexpr uint32_t Count = 33;
	}

	namespace Visibility
	{
		constexpr D3D12_SHADER_VISIBILITY PerFrame = D3D12_SHADER_VISIBILITY_ALL;
		constexpr D3D12_SHADER_VISIBILITY PerView = D3D12_SHADER_VISIBILITY_ALL;
		constexpr D3D12_SHADER_VISIBILITY PerObjectVS = D3D12_SHADER_VISIBILITY_VERTEX;
		constexpr D3D12_SHADER_VISIBILITY PerObjectPS = D3D12_SHADER_VISIBILITY_PIXEL;
		constexpr D3D12_SHADER_VISIBILITY TextureSRV = D3D12_SHADER_VISIBILITY_PIXEL;
		constexpr D3D12_SHADER_VISIBILITY SamplerTable = D3D12_SHADER_VISIBILITY_PIXEL;
	}
}
