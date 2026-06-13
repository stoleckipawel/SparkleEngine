#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "RHI/Public/Pipeline/RhiPipelineStateDesc.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <array>
#include <cstdint>
#include <string>

enum class PipelineRuntimeKind : std::uint8_t
{
	Graphics,
	Compute,
};

struct PipelineRuntimeKey final
{
	std::string PassName;
	std::string PackageDeclarationName;
	std::string PackageId;
	std::string BindingLayoutId;
	ERhiBackendApi Backend = ERhiBackendApi::Unknown;
	CookedShaderBinaryFormat RequiredBinaryFormat = CookedShaderBinaryFormat::Dxil;
	PipelineRuntimeKind PipelineKind = PipelineRuntimeKind::Graphics;
	ShaderStageMask ShaderStages = ShaderStageMask::None;
	CookedShaderPackageFeatureFlags RequiredFeatures = CookedShaderPackageFeatureFlags::None;
	CookedShaderPackageFeatureFlags PackageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint64_t ShaderPackageGeneration = 0;
	std::uint64_t ShaderPackageKey = 0;
	std::uint64_t SourceIdentityHash = 0;
	std::uint64_t BindingLayoutHash = 0;
	RhiVertexLayoutKind VertexLayout = RhiVertexLayoutKind::StaticMesh;
	bool HasPixelShader = false;
	bool RenderWireframe = false;
	ERhiCullMode CullMode = ERhiCullMode::Back;
	ERhiFrontFaceWinding FrontFaceWinding = ERhiFrontFaceWinding::Clockwise;
	RhiDepthTestDesc DepthTest = {};
	RhiStencilTestDesc StencilTest = {};
	std::array<PixelFormat, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 0;
	PixelFormat DepthStencilFormat = PixelFormat::Unknown;
};

const char* PipelineRuntimeKindToString(PipelineRuntimeKind kind) noexcept;
std::string FormatPipelineRuntimeKey(const PipelineRuntimeKey& key);
