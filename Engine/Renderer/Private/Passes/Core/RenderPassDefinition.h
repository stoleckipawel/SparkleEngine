#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

enum class RenderPassDefinitionPipelineKind
{
	Graphics,
	Compute,
};

struct RenderPassGraphicsPipelineDefinition final
{
	RhiVertexLayoutKind VertexLayout = RhiVertexLayoutKind::StaticMesh;
	bool RenderWireframe = false;
	ERhiCullMode CullMode = ERhiCullMode::Back;
	ERhiFrontFaceWinding FrontFaceWinding = ERhiFrontFaceWinding::Clockwise;
	RhiDepthTestDesc DepthTest = {};
	RhiStencilTestDesc StencilTest = {};
	std::array<PixelFormat, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 0;
	bool UsePresentColorFormat = false;
	PixelFormat DepthStencilFormat = PixelFormat::Unknown;
};

struct RenderPassDefinition final
{
	const char* PassName = nullptr;
	ShaderPackageDefinition ShaderPackage = {};
	RenderPassDefinitionPipelineKind PipelineKind = RenderPassDefinitionPipelineKind::Compute;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* BindingLayoutDebugName = L"RenderPass_BindingLayout";
	const wchar_t* PipelineDebugName = L"RenderPass_Pipeline";
	RenderPassGraphicsPipelineDefinition Graphics = {};
};
