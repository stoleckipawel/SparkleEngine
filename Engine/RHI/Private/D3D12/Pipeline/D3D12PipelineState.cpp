#include "PCH.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12VertexLayout.h"
#include "Config/DepthConvention.h"
#include "Shaders/CookedShaderPackageCache.h"
#include "Strings/StringUtils.h"

#include <cstdio>
#include <string>

static const auto g_pipelineStateLogger = Logging::GetOrCreateLogger("RHI.D3D12.Pipeline");

void D3D12PipelineState::SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept
{
	psoDesc.StreamOutput = {};
}

namespace
{
	D3D12_CULL_MODE ToD3D12CullMode(ERhiCullMode cullMode) noexcept
	{
		switch (cullMode)
		{
			case ERhiCullMode::None:
				return D3D12_CULL_MODE_NONE;
			case ERhiCullMode::Front:
				return D3D12_CULL_MODE_FRONT;
			case ERhiCullMode::Back:
			default:
				return D3D12_CULL_MODE_BACK;
		}
	}

	BOOL ToD3D12FrontCounterClockwise(ERhiFrontFaceWinding winding) noexcept
	{
		return winding == ERhiFrontFaceWinding::CounterClockwise ? TRUE : FALSE;
	}

	D3D12_STENCIL_OP ToD3D12StencilOp(RhiStencilOp op) noexcept
	{
		switch (op)
		{
			case RhiStencilOp::Zero:
				return D3D12_STENCIL_OP_ZERO;
			case RhiStencilOp::Replace:
				return D3D12_STENCIL_OP_REPLACE;
			case RhiStencilOp::IncrementClamp:
				return D3D12_STENCIL_OP_INCR_SAT;
			case RhiStencilOp::DecrementClamp:
				return D3D12_STENCIL_OP_DECR_SAT;
			case RhiStencilOp::Invert:
				return D3D12_STENCIL_OP_INVERT;
			case RhiStencilOp::IncrementWrap:
				return D3D12_STENCIL_OP_INCR;
			case RhiStencilOp::DecrementWrap:
				return D3D12_STENCIL_OP_DECR;
			case RhiStencilOp::Keep:
			default:
				return D3D12_STENCIL_OP_KEEP;
		}
	}

	std::span<const D3D12_INPUT_ELEMENT_DESC> ResolveVertexLayout(RhiVertexLayoutKind layoutKind) noexcept
	{
		switch (layoutKind)
		{
			case RhiVertexLayoutKind::StaticMesh:
			default:
				return D3D12VertexLayout::GetStaticMeshLayout();
		}
	}

	struct ResolvedD3D12ShaderStage final
	{
		D3D12_SHADER_BYTECODE Bytecode = {};
		std::string_view DebugArtifact = {};
	};

	ResolvedD3D12ShaderStage ResolveD3D12ShaderStage(const RhiShaderStageDesc& shaderDesc, std::string_view pipelineName, bool required)
	{
		if (!shaderDesc.IsValid())
		{
			if (required)
			{
				Diagnostics::Fail(
				    g_pipelineStateLogger,
				    __FILE__,
				    __LINE__,
				    std::format("Pipeline '{}' is missing a required cooked shader stage descriptor", pipelineName));
			}

			return {};
		}

		const LoadedShaderPackage& shaderPackage = *shaderDesc.Package;
		const CookedShaderBinaryRecord* shaderBinary =
		    shaderPackage.FindRuntimeBinaryRecord(shaderDesc.Stage, CookedShaderBinaryFormat::Dxil);
		if (shaderBinary == nullptr)
		{
			Diagnostics::Fail(
			    g_pipelineStateLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "Pipeline '{}' is missing a cooked DXIL/{} binary for shader stage '{}'",
			        pipelineName,
			        GetRuntimeShaderCodegenTarget(CookedShaderBinaryFormat::Dxil),
			        GetShaderStagePrefix(shaderDesc.Stage)));
		}

		const ShaderBytecode bytecode = shaderPackage.GetBytecode(*shaderBinary);
		if (!bytecode.IsValid())
		{
			Diagnostics::Fail(
			    g_pipelineStateLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "Pipeline '{}' has invalid cooked shader bytecode for stage '{}'",
			        pipelineName,
			        GetShaderStagePrefix(shaderDesc.Stage)));
		}

		ResolvedD3D12ShaderStage resolved{};
		resolved.Bytecode.pShaderBytecode = bytecode.Data;
		resolved.Bytecode.BytecodeLength = bytecode.Size;
		resolved.DebugArtifact = shaderPackage.ResolveString(shaderBinary->DebugArtifact);
		return resolved;
	}

	void LogShaderDebugArtifact(std::string_view stageName, std::string_view debugArtifact)
	{
		if (!debugArtifact.empty())
		{
			SPDLOG_LOGGER_ERROR(
			    g_pipelineStateLogger,
			    "{}",
			    std::format("D3D12 PSO stage '{}' debug artifact: {}", stageName, debugArtifact));
		}
	}

	std::string_view GetDiagnosticSeverityLabel(ERhiDiagnosticMessageSeverity severity) noexcept
	{
		switch (severity)
		{
			case ERhiDiagnosticMessageSeverity::Fatal:
				return "fatal";
			case ERhiDiagnosticMessageSeverity::Error:
				return "error";
			case ERhiDiagnosticMessageSeverity::Warning:
				return "warning";
			case ERhiDiagnosticMessageSeverity::Info:
				return "info";
			case ERhiDiagnosticMessageSeverity::Verbose:
			default:
				return "verbose";
		}
	}

	std::string_view GetDiagnosticCategoryLabel(ERhiDiagnosticMessageCategory category) noexcept
	{
		switch (category)
		{
			case ERhiDiagnosticMessageCategory::Validation:
				return "validation";
			case ERhiDiagnosticMessageCategory::Performance:
				return "performance";
			case ERhiDiagnosticMessageCategory::ResourceLifetime:
				return "resource-lifetime";
			case ERhiDiagnosticMessageCategory::Shader:
				return "shader";
			case ERhiDiagnosticMessageCategory::Driver:
				return "driver";
			case ERhiDiagnosticMessageCategory::Capture:
				return "capture";
			case ERhiDiagnosticMessageCategory::General:
			default:
				return "general";
		}
	}

	void LogDiagnosticMessage(const RhiDiagnosticMessage& diagnosticMessage) noexcept
	{
		const std::string message = std::format(
		    "D3D12 diagnostic [{}:{}] {}",
		    GetDiagnosticSeverityLabel(diagnosticMessage.Severity),
		    GetDiagnosticCategoryLabel(diagnosticMessage.Category),
		    diagnosticMessage.Text);

		switch (diagnosticMessage.Severity)
		{
			case ERhiDiagnosticMessageSeverity::Fatal:
			case ERhiDiagnosticMessageSeverity::Error:
				SPDLOG_LOGGER_ERROR(g_pipelineStateLogger, "{}", message);
				break;
			case ERhiDiagnosticMessageSeverity::Warning:
				SPDLOG_LOGGER_WARN(g_pipelineStateLogger, "{}", message);
				break;
			case ERhiDiagnosticMessageSeverity::Info:
				break;
			case ERhiDiagnosticMessageSeverity::Verbose:
			default:
				break;
		}
	}
}

void D3D12PipelineState::SetRasterizerState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    bool bRenderWireframe,
    ERhiCullMode cullMode,
    ERhiFrontFaceWinding frontFaceWinding) noexcept
{
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	auto& rs = psoDesc.RasterizerState;
	rs = {};
	rs.FillMode = bRenderWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rs.CullMode = ToD3D12CullMode(cullMode);
	rs.FrontCounterClockwise = ToD3D12FrontCounterClockwise(frontFaceWinding);
	rs.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rs.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rs.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rs.DepthClipEnable = TRUE;
	rs.MultisampleEnable = FALSE;
	rs.AntialiasedLineEnable = FALSE;
	rs.ForcedSampleCount = 0;
	rs.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void D3D12PipelineState::SetRenderTargetBlendState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc) noexcept
{
	psoDesc.BlendState = {};
	for (D3D12_RENDER_TARGET_BLEND_DESC& renderTargetBlend : psoDesc.BlendState.RenderTarget)
	{
		renderTargetBlend = blendDesc;
	}
}

void D3D12PipelineState::SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiDepthTestDesc depthDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds = {};
	ds.DepthEnable = depthDesc.DepthEnable ? TRUE : FALSE;
	ds.DepthWriteMask = depthDesc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D12TypeConversions::ToComparisonFunc(depthDesc.DepthFunc);
}

void D3D12PipelineState::SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiStencilTestDesc stencilDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds.StencilEnable = stencilDesc.StencilEnable ? TRUE : FALSE;
	ds.StencilReadMask = stencilDesc.StencilReadMask;
	ds.StencilWriteMask = stencilDesc.StencilWriteMask;

	ds.FrontFace.StencilFunc = D3D12TypeConversions::ToComparisonFunc(stencilDesc.FrontFaceStencilFunc);
	ds.FrontFace.StencilFailOp = ToD3D12StencilOp(stencilDesc.FrontFaceStencilFailOp);
	ds.FrontFace.StencilDepthFailOp = ToD3D12StencilOp(stencilDesc.FrontFaceStencilDepthFailOp);
	ds.FrontFace.StencilPassOp = ToD3D12StencilOp(stencilDesc.FrontFaceStencilPassOp);

	ds.BackFace.StencilFunc = D3D12TypeConversions::ToComparisonFunc(stencilDesc.BackFaceStencilFunc);
	ds.BackFace.StencilFailOp = ToD3D12StencilOp(stencilDesc.BackFaceStencilFailOp);
	ds.BackFace.StencilDepthFailOp = ToD3D12StencilOp(stencilDesc.BackFaceStencilDepthFailOp);
	ds.BackFace.StencilPassOp = ToD3D12StencilOp(stencilDesc.BackFaceStencilPassOp);
}

D3D12PipelineState::D3D12PipelineState(D3D12Rhi& rhi, const GraphicsPipelineStateDesc& desc) : m_rhi(rhi)
{
	Create(desc);
}

D3D12PipelineState::D3D12PipelineState(D3D12Rhi& rhi, const ComputePipelineStateDesc& desc) : m_rhi(rhi)
{
	Create(desc);
}

void D3D12PipelineState::Create(const GraphicsPipelineStateDesc& desc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_GraphicsPipelineState";
	const ResolvedD3D12ShaderStage vertexShader = ResolveD3D12ShaderStage(desc.VertexShader, pipelineName, true);
	const ResolvedD3D12ShaderStage pixelShader = ResolveD3D12ShaderStage(desc.PixelShader, pipelineName, false);

	const std::span<const D3D12_INPUT_ELEMENT_DESC> vertexLayout = ResolveVertexLayout(desc.VertexLayout);
	psoDesc.InputLayout.NumElements = static_cast<UINT>(vertexLayout.size());
	psoDesc.InputLayout.pInputElementDescs = vertexLayout.data();
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	const auto* bindingLayout = static_cast<const D3D12BindingLayout*>(desc.BindingLayout);
	psoDesc.pRootSignature = bindingLayout != nullptr ? bindingLayout->GetRootSignature().GetRaw() : nullptr;

	psoDesc.VS = vertexShader.Bytecode;
	psoDesc.PS = pixelShader.Bytecode;

	SetRasterizerState(psoDesc, desc.RenderWireframe, desc.CullMode, desc.FrontFaceWinding);

	SetStreamOutput(psoDesc);

	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};
	rtBlend.BlendEnable = FALSE;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlend = D3D12_BLEND_ONE;
	rtBlend.DestBlend = D3D12_BLEND_ZERO;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlend.LogicOpEnable = FALSE;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	SetRenderTargetBlendState(psoDesc, rtBlend);

	SetDepthTestState(psoDesc, desc.DepthTest);
	SetStencilTestState(psoDesc, desc.StencilTest);

	psoDesc.NumRenderTargets = desc.RenderTargetCount;
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.RenderTargetCount; ++renderTargetIndex)
	{
		psoDesc.RTVFormats[renderTargetIndex] = D3D12TypeConversions::ToDxgiFormat(desc.RenderTargetFormats[renderTargetIndex]);
	}
	psoDesc.DSVFormat = D3D12TypeConversions::ToDxgiFormat(desc.DepthStencilFormat);

	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	HRESULT hr = m_rhi.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		LogShaderDebugArtifact("vs", vertexShader.DebugArtifact);
		LogShaderDebugArtifact("ps", pixelShader.DebugArtifact);
		HandlePsoCreateFailure(hr);
	}

	m_pso->SetName(desc.DebugName);
}

void D3D12PipelineState::Create(const ComputePipelineStateDesc& desc)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_ComputePipelineState";
	const ResolvedD3D12ShaderStage computeShader = ResolveD3D12ShaderStage(desc.ComputeShader, pipelineName, true);
	const auto* bindingLayout = static_cast<const D3D12BindingLayout*>(desc.BindingLayout);
	psoDesc.pRootSignature = bindingLayout != nullptr ? bindingLayout->GetRootSignature().GetRaw() : nullptr;
	psoDesc.CS = computeShader.Bytecode;
	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr = m_rhi.GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		LogShaderDebugArtifact("cs", computeShader.DebugArtifact);
		HandlePsoCreateFailure(hr);
	}

	m_pso->SetName(desc.DebugName);
}

void D3D12PipelineState::HandlePsoCreateFailure(HRESULT hr) const noexcept
{
	m_rhi.CollectCrashDiagnostics();

	RhiDiagnosticMessage diagnosticMessage{};
	while (m_rhi.TryPopDebugMessage(diagnosticMessage))
	{
		LogDiagnosticMessage(diagnosticMessage);
	}

	char buf[256];
	std::snprintf(buf, sizeof(buf), "Failed To Create PSO. HRESULT: 0x%08X", static_cast<unsigned int>(hr));
	Diagnostics::Fail(g_pipelineStateLogger, __FILE__, __LINE__, buf);
}

D3D12PipelineState::~D3D12PipelineState() noexcept
{
	m_pso.Reset();
}
