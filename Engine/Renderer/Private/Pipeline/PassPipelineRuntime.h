#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <array>
#include <cstdint>

class RenderBindingLayout;
class RenderPipeline;

struct GraphicsShaderPipelineState final
{
	RhiVertexLayoutKind VertexLayout = RhiVertexLayoutKind::StaticMesh;
	bool RenderWireframe = false;
	ERhiCullMode CullMode = ERhiCullMode::Back;
	ERhiFrontFaceWinding FrontFaceWinding = ERhiFrontFaceWinding::Clockwise;
	RhiDepthTestDesc DepthTest = {};
	RhiStencilTestDesc StencilTest = {};
	std::array<PixelFormat, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 0;
	PixelFormat DepthStencilFormat = PixelFormat::Unknown;

	bool operator==(const GraphicsShaderPipelineState&) const noexcept = default;
};

struct RasterPassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
	RenderPipeline* WireframePipeline = nullptr;
	RenderPipeline* TwoSidedPipeline = nullptr;
};

struct ComputePassPipelineRuntime
{
	RenderBindingLayout& BindingLayout;
	RenderPipeline& Pipeline;
};
