#include "PCH.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "Strings/StringUtils.h"
#include "Validation/RhiContract.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

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
				return D3D12_CULL_MODE_BACK;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported cull mode.");
	}

	static BOOL ToD3D12FrontCounterClockwise(ERhiFrontFaceWinding winding) noexcept
	{
		switch (winding)
		{
			case ERhiFrontFaceWinding::Clockwise:
				return FALSE;
			case ERhiFrontFaceWinding::CounterClockwise:
				return TRUE;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported front-face winding.");
	}

	static D3D12_FILL_MODE ToD3D12FillMode(RhiFillMode fillMode) noexcept
	{
		switch (fillMode)
		{
			case RhiFillMode::Solid:
				return D3D12_FILL_MODE_SOLID;
			case RhiFillMode::Wireframe:
				return D3D12_FILL_MODE_WIREFRAME;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported fill mode.");
	}

	static D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12PrimitiveTopologyType(RhiPrimitiveTopology topology) noexcept
	{
		switch (topology)
		{
			case RhiPrimitiveTopology::TriangleList:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported primitive topology.");
	}

	static D3D12_BLEND ToD3D12Blend(RhiBlendFactor factor) noexcept
	{
		switch (factor)
		{
			case RhiBlendFactor::Zero:
				return D3D12_BLEND_ZERO;
			case RhiBlendFactor::SourceAlpha:
				return D3D12_BLEND_SRC_ALPHA;
			case RhiBlendFactor::InverseSourceAlpha:
				return D3D12_BLEND_INV_SRC_ALPHA;
			case RhiBlendFactor::One:
				return D3D12_BLEND_ONE;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported blend factor.");
	}

	static D3D12_BLEND_OP ToD3D12BlendOperation(RhiBlendOperation operation) noexcept
	{
		switch (operation)
		{
			case RhiBlendOperation::Add:
				return D3D12_BLEND_OP_ADD;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported blend operation.");
	}

	static DXGI_FORMAT ToD3D12VertexFormat(RhiVertexElementFormat format) noexcept
	{
		switch (format)
		{
			case RhiVertexElementFormat::Float2:
				return DXGI_FORMAT_R32G32_FLOAT;
			case RhiVertexElementFormat::Float4:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;
			case RhiVertexElementFormat::Float3:
				return DXGI_FORMAT_R32G32B32_FLOAT;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported vertex element format.");
	}

	static const char* ToD3D12Semantic(RhiVertexSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case RhiVertexSemantic::TexCoord:
				return "TEXCOORD";
			case RhiVertexSemantic::Normal:
				return "NORMAL";
			case RhiVertexSemantic::Tangent:
				return "TANGENT";
			case RhiVertexSemantic::Position:
				return "POSITION";
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported vertex semantic.");
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
				return D3D12_STENCIL_OP_KEEP;
		}
		Diagnostics::Fatal(g_pipelineLogger, __FILE__, __LINE__, "D3D12 received an unsupported stencil operation.");
	}

	static std::vector<D3D12_INPUT_ELEMENT_DESC> BuildVertexInput(const RhiVertexInputDeclaration& declaration)
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> result;
		result.reserve(declaration.ElementCount);
		for (std::uint32_t index = 0; index < declaration.ElementCount; ++index)
		{
			const RhiVertexInputElement& element = declaration.Elements[index];
			const RhiVertexInputBinding* binding = nullptr;
			for (std::uint32_t bindingIndex = 0; bindingIndex < declaration.BindingCount; ++bindingIndex)
			{
				if (declaration.Bindings[bindingIndex].Binding == element.Binding)
				{
					binding = &declaration.Bindings[bindingIndex];
					break;
				}
			}
			assert(binding != nullptr && "Neutral graphics pipeline validation must reject missing vertex bindings.");
			result.push_back(
			    D3D12_INPUT_ELEMENT_DESC{
			        ToD3D12Semantic(element.Semantic),
			        element.SemanticIndex,
			        ToD3D12VertexFormat(element.Format),
			        element.Binding,
			        element.OffsetInBytes,
			        binding->PerInstance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			        binding->PerInstance ? 1u : 0u});
		}
		return result;
	}

	struct ResolvedD3D12ShaderStage final
	{
		D3D12_SHADER_BYTECODE Bytecode = {};
	};

	static ResolvedD3D12ShaderStage ResolveD3D12ShaderStage(const RhiShaderStageDesc& shaderDesc, std::string_view pipelineName)
	{
		if (!shaderDesc.IsValid())
		{
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

void D3D12Pipeline::SetRasterizerState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const RhiRasterizerState& rasterizer) noexcept
{
	auto& rs = psoDesc.RasterizerState;
	rs = {};
	rs.FillMode = D3D12PipelineImplementation::ToD3D12FillMode(rasterizer.FillMode);
	rs.CullMode = D3D12PipelineImplementation::ToD3D12CullMode(rasterizer.CullMode);
	rs.FrontCounterClockwise = D3D12PipelineImplementation::ToD3D12FrontCounterClockwise(rasterizer.FrontFaceWinding);
	rs.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rs.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rs.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rs.DepthClipEnable = rasterizer.DepthClipEnable ? TRUE : FALSE;
	rs.MultisampleEnable = FALSE;
	rs.AntialiasedLineEnable = FALSE;
	rs.ForcedSampleCount = 0;
	rs.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void D3D12Pipeline::SetRenderTargetBlendState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    const RhiBlendState& blend,
    std::uint32_t colorAttachmentCount) noexcept
{
	psoDesc.BlendState = {};
	psoDesc.BlendState.AlphaToCoverageEnable = blend.AlphaToCoverageEnable ? TRUE : FALSE;
	psoDesc.BlendState.IndependentBlendEnable = blend.IndependentBlendEnable ? TRUE : FALSE;
	for (std::uint32_t index = 0; index < colorAttachmentCount; ++index)
	{
		const RhiBlendTargetState& source = blend.Targets[blend.IndependentBlendEnable ? index : 0];
		D3D12_RENDER_TARGET_BLEND_DESC& target = psoDesc.BlendState.RenderTarget[index];
		target.BlendEnable = source.BlendEnable ? TRUE : FALSE;
		target.SrcBlend = D3D12PipelineImplementation::ToD3D12Blend(source.SourceColor);
		target.DestBlend = D3D12PipelineImplementation::ToD3D12Blend(source.DestinationColor);
		target.BlendOp = D3D12PipelineImplementation::ToD3D12BlendOperation(source.ColorOperation);
		target.SrcBlendAlpha = D3D12PipelineImplementation::ToD3D12Blend(source.SourceAlpha);
		target.DestBlendAlpha = D3D12PipelineImplementation::ToD3D12Blend(source.DestinationAlpha);
		target.BlendOpAlpha = D3D12PipelineImplementation::ToD3D12BlendOperation(source.AlphaOperation);
		target.RenderTargetWriteMask = source.ColorWriteMask;
	}
}

void D3D12Pipeline::SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const RhiDepthState& depthDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds = {};
	ds.DepthEnable = depthDesc.DepthEnable ? TRUE : FALSE;
	ds.DepthWriteMask = depthDesc.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D12TypeConversions::ToComparisonFunc(depthDesc.DepthFunc);
}

void D3D12Pipeline::SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const RhiStencilState& stencilDesc) noexcept
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

D3D12Pipeline::D3D12Pipeline(D3D12Rhi& rhi, const GraphicsPipelineDesc& desc) :
    m_rhi(rhi)
{
	Create(desc);
}

D3D12Pipeline::D3D12Pipeline(D3D12Rhi& rhi, const ComputePipelineDesc& desc) :
    m_rhi(rhi)
{
	Create(desc);
}

void D3D12Pipeline::Create(const GraphicsPipelineDesc& desc)
{
	RhiContract::ValidateGraphicsPipelineDesc(desc);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_GraphicsPipeline";
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage vertexShader =
	    D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.VertexShader, pipelineName);
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage pixelShader =
	    D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.PixelShader, pipelineName);

	const std::vector<D3D12_INPUT_ELEMENT_DESC> vertexLayout = D3D12PipelineImplementation::BuildVertexInput(desc.VertexInput);
	psoDesc.InputLayout.NumElements = static_cast<UINT>(vertexLayout.size());
	psoDesc.InputLayout.pInputElementDescs = vertexLayout.data();
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	const auto* bindingLayout = static_cast<const D3D12BindingLayout*>(desc.BindingLayout);
	psoDesc.pRootSignature = bindingLayout != nullptr ? bindingLayout->GetRootSignature().GetRaw() : nullptr;

	psoDesc.VS = vertexShader.Bytecode;
	psoDesc.PS = pixelShader.Bytecode;

	psoDesc.PrimitiveTopologyType = D3D12PipelineImplementation::ToD3D12PrimitiveTopologyType(desc.PrimitiveTopology);
	SetRasterizerState(psoDesc, desc.Rasterizer);

	SetStreamOutput(psoDesc);

	SetRenderTargetBlendState(psoDesc, desc.Blend, desc.ColorAttachmentCount);

	SetDepthTestState(psoDesc, desc.Depth);
	SetStencilTestState(psoDesc, desc.Stencil);

	psoDesc.NumRenderTargets = desc.ColorAttachmentCount;
	for (std::uint32_t renderTargetIndex = 0; renderTargetIndex < desc.ColorAttachmentCount; ++renderTargetIndex)
	{
		psoDesc.RTVFormats[renderTargetIndex] = D3D12TypeConversions::ToDxgiFormat(desc.ColorAttachmentFormats[renderTargetIndex]);
	}
	psoDesc.DSVFormat = D3D12TypeConversions::ToDxgiFormat(desc.DepthStencilAttachmentFormat);

	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = desc.SampleCount;
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
	RhiContract::ValidateComputePipelineDesc(desc);
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	const std::string pipelineName = desc.DebugName != nullptr ? Strings::ToNarrow(desc.DebugName) : "RHI_ComputePipeline";
	const D3D12PipelineImplementation::ResolvedD3D12ShaderStage computeShader =
	    D3D12PipelineImplementation::ResolveD3D12ShaderStage(desc.ComputeShader, pipelineName);
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
