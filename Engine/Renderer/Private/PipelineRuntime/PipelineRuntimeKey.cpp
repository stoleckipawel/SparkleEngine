#include "../PCH.h"
#include "PipelineRuntimeKey.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <format>
#include <string>

namespace
{
	const char* FormatBool(bool value) noexcept
	{
		return value ? "true" : "false";
	}

	const char* FormatCullMode(ERhiCullMode mode) noexcept
	{
		switch (mode)
		{
			case ERhiCullMode::None:
				return "none";
			case ERhiCullMode::Front:
				return "front";
			case ERhiCullMode::Back:
				return "back";
		}

		return "unknown";
	}

	const char* FormatFrontFaceWinding(ERhiFrontFaceWinding winding) noexcept
	{
		switch (winding)
		{
			case ERhiFrontFaceWinding::Clockwise:
				return "clockwise";
			case ERhiFrontFaceWinding::CounterClockwise:
				return "counterClockwise";
		}

		return "unknown";
	}

	const char* FormatCompareOp(CompareOp op) noexcept
	{
		switch (op)
		{
			case CompareOp::Never:
				return "never";
			case CompareOp::Less:
				return "less";
			case CompareOp::Equal:
				return "equal";
			case CompareOp::LessOrEqual:
				return "lessOrEqual";
			case CompareOp::Greater:
				return "greater";
			case CompareOp::NotEqual:
				return "notEqual";
			case CompareOp::GreaterOrEqual:
				return "greaterOrEqual";
			case CompareOp::Always:
				return "always";
		}

		return "unknown";
	}

	const char* FormatStencilOp(RhiStencilOp op) noexcept
	{
		switch (op)
		{
			case RhiStencilOp::Keep:
				return "keep";
			case RhiStencilOp::Zero:
				return "zero";
			case RhiStencilOp::Replace:
				return "replace";
			case RhiStencilOp::IncrementClamp:
				return "incrementClamp";
			case RhiStencilOp::DecrementClamp:
				return "decrementClamp";
			case RhiStencilOp::Invert:
				return "invert";
			case RhiStencilOp::IncrementWrap:
				return "incrementWrap";
			case RhiStencilOp::DecrementWrap:
				return "decrementWrap";
		}

		return "unknown";
	}

	const char* FormatVertexLayout(RhiVertexLayoutKind layout) noexcept
	{
		switch (layout)
		{
			case RhiVertexLayoutKind::StaticMesh:
				return "staticMesh";
		}

		return "unknown";
	}

	std::string FormatFeatureFlags(CookedShaderPackageFeatureFlags flags)
	{
		std::string result;
		if (HasCookedShaderPackageFeature(flags, CookedShaderPackageFeatureFlags::UsesInlineRayQuery))
		{
			result += "inlineRayQuery";
		}
		if (HasCookedShaderPackageFeature(flags, CookedShaderPackageFeatureFlags::UsesAccelerationStructure))
		{
			if (!result.empty())
			{
				result += "|";
			}
			result += "accelerationStructure";
		}
		if (HasCookedShaderPackageFeature(flags, CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress))
		{
			if (!result.empty())
			{
				result += "|";
			}
			result += "accelerationStructureDeviceAddress";
		}
		if (result.empty())
		{
			result = "none";
		}
		return result;
	}

	std::string FormatRenderTargetFormats(const PipelineRuntimeKey& key)
	{
		std::string result;
		for (std::uint32_t index = 0; index < key.RenderTargetCount && index < key.RenderTargetFormats.size(); ++index)
		{
			if (!result.empty())
			{
				result += ",";
			}
			result += PixelFormatName(key.RenderTargetFormats[index]);
		}
		if (result.empty())
		{
			result = "none";
		}
		return result;
	}
}

const char* PipelineRuntimeKindToString(PipelineRuntimeKind kind) noexcept
{
	switch (kind)
	{
		case PipelineRuntimeKind::Graphics:
			return "graphics";
		case PipelineRuntimeKind::Compute:
			return "compute";
	}

	return "unknown";
}

std::string FormatPipelineRuntimeKey(const PipelineRuntimeKey& key)
{
	return std::format(
	    "pass='{}' packageDeclaration='{}' package='{}' bindingLayout='{}' backend='{}' requiredFormat='{}' kind='{}' stages='{}' "
	    "requiredFeatures='{}' packageFeatures='{}' generation={} packageKey=0x{:016X} sourceHash=0x{:016X} bindingLayoutHash=0x{:016X} "
	    "vertexLayout='{}' hasPixelShader='{}' wireframe='{}' cull='{}' winding='{}' depthEnable='{}' depthWrite='{}' depthFunc='{}' "
	    "stencilEnable='{}' stencilReadMask=0x{:02X} stencilWriteMask=0x{:02X} stencilFront='{}:{}:{}:{}' stencilBack='{}:{}:{}:{}' "
	    "renderTargets='{}' depthStencil='{}'",
	    key.PassName,
	    key.PackageDeclarationName,
	    key.PackageId,
	    key.BindingLayoutId,
	    RhiBackendApiToString(key.Backend),
	    CookedShaderBinaryFormatToString(key.RequiredBinaryFormat),
	    PipelineRuntimeKindToString(key.PipelineKind),
	    FormatShaderStageMask(key.ShaderStages),
	    FormatFeatureFlags(key.RequiredFeatures),
	    FormatFeatureFlags(key.PackageFeatures),
	    key.ShaderPackageGeneration,
	    key.ShaderPackageKey,
	    key.SourceIdentityHash,
	    key.BindingLayoutHash,
	    FormatVertexLayout(key.VertexLayout),
	    FormatBool(key.HasPixelShader),
	    FormatBool(key.RenderWireframe),
	    FormatCullMode(key.CullMode),
	    FormatFrontFaceWinding(key.FrontFaceWinding),
	    FormatBool(key.DepthTest.DepthEnable),
	    FormatBool(key.DepthTest.DepthWriteEnable),
	    FormatCompareOp(key.DepthTest.DepthFunc),
	    FormatBool(key.StencilTest.StencilEnable),
	    key.StencilTest.StencilReadMask,
	    key.StencilTest.StencilWriteMask,
	    FormatCompareOp(key.StencilTest.FrontFaceStencilFunc),
	    FormatStencilOp(key.StencilTest.FrontFaceStencilFailOp),
	    FormatStencilOp(key.StencilTest.FrontFaceStencilDepthFailOp),
	    FormatStencilOp(key.StencilTest.FrontFaceStencilPassOp),
	    FormatCompareOp(key.StencilTest.BackFaceStencilFunc),
	    FormatStencilOp(key.StencilTest.BackFaceStencilFailOp),
	    FormatStencilOp(key.StencilTest.BackFaceStencilDepthFailOp),
	    FormatStencilOp(key.StencilTest.BackFaceStencilPassOp),
	    FormatRenderTargetFormats(key),
	    PixelFormatName(key.DepthStencilFormat));
}
