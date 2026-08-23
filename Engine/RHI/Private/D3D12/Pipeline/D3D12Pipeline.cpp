#include "PCH.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12VertexLayout.h"
#include "Config/DepthConvention.h"
#include "Strings/StringUtils.h"

#include <cstdio>
#include <string>

static const auto g_pipelineLogger = Logging::GetOrCreateLogger("RHI.D3D12.Pipeline");

void D3D12Pipeline::SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept
{
	psoDesc.StreamOutput = {};
}

class D3D12PipelineImplementation final
{
  public:
	static D3D12_CULL_MODE ToD3D12CullMode(ERhiCullMode cullMode) noexcept
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

	static BOOL ToD3D12FrontCounterClockwise(ERhiFrontFaceWinding winding) noexcept
	{
		return winding == ERhiFrontFaceWinding::CounterClockwise ? TRUE : FALSE;
	}

	static D3D12_STENCIL_OP ToD3D12StencilOp(RhiStencilOp op) noexcept
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

	static std::span<const D3D12_INPUT_ELEMENT_DESC> ResolveVertexLayout(RhiVertexLayoutKind layoutKind) noexcept
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
	};

	static ResolvedD3D12ShaderStage ResolveD3D12ShaderStage(const RhiShaderStageDesc& shaderDesc, std::string_view pipelineName, bool required)
	{
		if (!shaderDesc.IsValid())
		{
			if (required)
			{
				Diagnostics::Fatal(
				    g_pipelineLogger,
				    __FILE__,
				    __LINE__,
				    std::format("Pipeline '{}' is missing a required cooked shader stage descriptor", pipelineName));
			}

			return {};
		}

		const ResolvedShader& shader = *shaderDesc.Shader;
		if (shader.Entry->BinaryFormat != ShaderBinaryFormat::Dxil)
		{
			Diagnostics::Fatal(
			    g_pipelineLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "Pipeline '{}' received non-DXIL code for shader stage '{}'",
			        pipelineName,
			        GetShaderStagePrefix(shader.Entry->Stage)));
		}

		const ShaderBytecode bytecode = shader.GetBytecode();
		if (!bytecode.IsValid())
		{
			Diagnostics::Fatal(
			    g_pipelineLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "Pipeline '{}' has invalid cooked shader bytecode for stage '{}'",
			        pipelineName,
			        GetShaderStagePrefix(shader.Entry->Stage)));
		}

		ResolvedD3D12ShaderStage resolved{};
		resolved.Bytecode.pShaderBytecode = bytecode.Data;
		resolved.Bytecode.BytecodeLength = bytecode.Size;
		return resolved;
	}

	static std::string_view GetDiagnosticSeverityLabel(ERhiDiagnosticMessageSeverity severity) noexcept
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

	static std::string_view GetDiagnosticCategoryLabel(ERhiDiagnosticMessageCategory category) noexcept
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

	static void LogDiagnosticMessage(const RhiDiagnosticMessage& diagnosticMessage) noexcept
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
				SPDLOG_LOGGER_ERROR(g_pipelineLogger, "{}", message);
				break;
			case ERhiDiagnosticMessageSeverity::Warning:
				SPDLOG_LOGGER_WARN(g_pipelineLogger, "{}", message);
				break;
			case ERhiDiagnosticMessageSeverity::Info:
				break;
			case ERhiDiagnosticMessageSeverity::Verbose:
			default:
				break;
		}
	}
};

void D3D12Pipeline::SetRasterizerState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    bool bRenderWireframe,
    ERhiCullMode cullMode,
    ERhiFrontFaceWinding frontFaceWinding) noexcept
{
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	auto& rs = psoDesc.RasterizerState;
	rs = {};
	rs.FillMode = bRenderWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rs.CullMode = D3D12PipelineImplementation::ToD3D12CullMode(cullMode);
	rs.FrontCounterClockwise = D3D12PipelineImplementation::ToD3D12FrontCounterClockwise(frontFaceWinding);
	rs.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rs.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rs.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rs.DepthClipEnable = TRUE;
	rs.MultisampleEnable = FALSE;
	rs.AntialiasedLineEnable = FALSE;
	rs.ForcedSampleCount = 0;
	rs.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void D3D12Pipeline::SetRenderTargetBlendState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc) noexcept
{
	psoDesc.BlendState = {};
	for (D3D12_RENDER_TARGET_BLEND_DESC& renderTargetBlend : psoDesc.BlendState.RenderTarget)
	{
		renderTargetBlend = blendDesc;
	}
}

void D3D12Pipeline::SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiDepthTestDesc depthDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds = {};
	ds.DepthEnable = depthDesc.DepthEnable ? TRUE : FALSE;
	ds.DepthWriteMask = depthDesc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D12TypeConversions::ToComparisonFunc(depthDesc.DepthFunc);
}

void D3D12Pipeline::SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiStencilTestDesc stencilDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds.StencilEnable = stencilDesc.StencilEnable ? TRUE : FALSE;
	ds.StencilReadMask = stencilDesc.StencilReadMask;
	ds.StencilWriteMask = stencilDesc.StencilWriteMask;

	ds.FrontFace.StencilFunc = D3D12TypeConversions::ToComparisonFunc(stencilDesc.FrontFaceStencilFunc);
	ds.FrontFace.StencilFailOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.FrontFaceStencilFailOp);
	ds.FrontFace.StencilDepthFailOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.FrontFaceStencilDepthFailOp);
	ds.FrontFace.StencilPassOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.FrontFaceStencilPassOp);

	ds.BackFace.StencilFunc = D3D12TypeConversions::ToComparisonFunc(stencilDesc.BackFaceStencilFunc);
	ds.BackFace.StencilFailOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.BackFaceStencilFailOp);
	ds.BackFace.StencilDepthFailOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.BackFaceStencilDepthFailOp);
	ds.BackFace.StencilPassOp = D3D12PipelineImplementation::ToD3D12StencilOp(stencilDesc.BackFaceStencilPassOp);
}

D3D12Pipeline::D3D12Pipeline(D3D12Rhi& rhi, const GraphicsPipelineDesc& desc) : m_rhi(rhi)
{
	Create(desc);
}

D3D12Pipeline::D3D12Pipeline(D3D12Rhi& rhi, const ComputePipelineDesc& desc) : m_rhi(rhi)
{
	Create(desc);
}

void D3D12Pipeline::Create(const GraphicsPipelineDesc& desc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_GraphicsPipeline";
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage vertexShader = D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.VertexShader, pipelineName, true);
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage pixelShader = D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.PixelShader, pipelineName, false);

	const std::span<const D3D12_INPUT_ELEMENT_DESC> vertexLayout = D3D12PipelineImplementation::ResolveVertexLayout(desc.VertexLayout);
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
		HandlePsoCreateFailure(hr);
	}

	m_pso->SetName(desc.DebugName);
}

void D3D12Pipeline::Create(const ComputePipelineDesc& desc)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_ComputePipeline";
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage computeShader = D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.ComputeShader, pipelineName, true);
	const auto* bindingLayout = static_cast<const D3D12BindingLayout*>(desc.BindingLayout);
	psoDesc.pRootSignature = bindingLayout != nullptr ? bindingLayout->GetRootSignature().GetRaw() : nullptr;
	psoDesc.CS = computeShader.Bytecode;
	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr = m_rhi.GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		HandlePsoCreateFailure(hr);
	}

	m_pso->SetName(desc.DebugName);
}

void D3D12Pipeline::HandlePsoCreateFailure(HRESULT hr) const noexcept
{
	m_rhi.CollectCrashDiagnostics();

	RhiDiagnosticMessage diagnosticMessage{};
	while (m_rhi.TryPopDebugMessage(diagnosticMessage))
	{
		D3D12PipelineImplementation::LogDiagnosticMessage(diagnosticMessage);
	}

	char buf[256];
	std::snprintf(buf, sizeof(buf), "Failed To Create PSO. HRESULT: 0x%08X", static_cast<unsigned int>(hr));
	Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, buf);
}

D3D12Pipeline::~D3D12Pipeline() noexcept
{
	m_pso.Reset();
}
