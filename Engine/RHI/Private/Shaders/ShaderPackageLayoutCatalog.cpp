#include "PCH.h"

#include "Shaders/ShaderPackageLayoutCatalog.h"

#include "Core/Public/Strings/StringUtils.h"

namespace ShaderPackageLayouts::Details
{
	PassParameterLayout BuildForwardOpaqueLayout()
	{
		PassParameterLayout layout("ForwardOpaque");
		layout.Add<ReadTexture>("MaterialTextures", ShaderStageVisibility::Pixel, kForwardOpaqueMaterialTextureCount);
		layout.Add<RenderTarget>("BackBuffer", ShaderStageVisibility::AllGraphics);
		layout.Add<DepthTarget>("MainDepth", ShaderStageVisibility::AllGraphics);
		layout.Add<ReadTexture>("ShadowMap0", ShaderStageVisibility::Pixel);
		layout.Add<ReadTexture>("ShadowMap1", ShaderStageVisibility::Pixel);
		layout.Add<ReadTexture>("ShadowMap2", ShaderStageVisibility::Pixel);
		layout.Add<ReadTexture>("ShadowMap3", ShaderStageVisibility::Pixel);
		layout.Add<UniformData<PerFrameConstantBufferData>>("PerFrame", ShaderStageVisibility::AllGraphics);
		layout.Add<UniformData<PerViewConstantBufferData>>("PerView", ShaderStageVisibility::AllGraphics);
		layout.Add<SamplerSet>("SamplerTable", ShaderStageVisibility::Pixel);
		layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
		layout.Add<UniformData<PerObjectPSConstantBufferData>>("PerObjectPS", ShaderStageVisibility::Pixel);
		return layout;
	}

	PassParameterLayout BuildShadowOpaqueLayout()
	{
		PassParameterLayout layout("ShadowOpaque");
		layout.Add<RenderTarget>("ShadowColor", ShaderStageVisibility::AllGraphics);
		layout.Add<DepthTarget>("ShadowDepth", ShaderStageVisibility::AllGraphics);
		layout.Add<UniformData<PerFrameConstantBufferData>>("PerFrame", ShaderStageVisibility::AllGraphics);
		layout.Add<UniformData<PerViewConstantBufferData>>("PerView", ShaderStageVisibility::AllGraphics);
		layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
		return layout;
	}

	PassParameterLayout BuildComputeClearLayout()
	{
		PassParameterLayout layout("ComputeClear");
		layout.Add<RWTexture>("Output", ShaderStageVisibility::Compute);
		return layout;
	}
}

bool ShaderPackageLayouts::TryBuild(
	std::string_view bindingLayoutId,
	PassParameterLayout& outLayout,
	std::string& outErrorMessage)
{
	const std::string normalizedBindingLayoutId =
	    Engine::Strings::ToLowerCopy(Engine::Strings::TrimAsciiWhitespace(bindingLayoutId));

	if (normalizedBindingLayoutId == "forwardopaque")
	{
		outLayout = Details::BuildForwardOpaqueLayout();
		outErrorMessage.clear();
		return true;
	}

	if (normalizedBindingLayoutId == "shadowopaque")
	{
		outLayout = Details::BuildShadowOpaqueLayout();
		outErrorMessage.clear();
		return true;
	}

	if (normalizedBindingLayoutId == "computeclear")
	{
		outLayout = Details::BuildComputeClearLayout();
		outErrorMessage.clear();
		return true;
	}

	outErrorMessage = "Unknown shader package binding layout '" + std::string(bindingLayoutId) + "'";
	return false;
}