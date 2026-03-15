#pragma once

#include <memory>

class D3D12PipelineState;
class D3D12Rhi;
class D3D12RootSignature;
class ShaderCompileResult;

class PipelineStateManager final
{
  public:
	explicit PipelineStateManager(D3D12Rhi& rhi) noexcept;
	~PipelineStateManager() noexcept;

	PipelineStateManager(const PipelineStateManager&) = delete;
	PipelineStateManager& operator=(const PipelineStateManager&) = delete;
	PipelineStateManager(PipelineStateManager&&) = delete;
	PipelineStateManager& operator=(PipelineStateManager&&) = delete;

	D3D12RootSignature& GetRootSignature() const noexcept;
	D3D12PipelineState& GetPipelineState() const noexcept;

  private:
	void CreateRootSignature();
	void CompileShaders();
	void CreatePipelineState();

	D3D12Rhi* m_rhi = nullptr;
	std::unique_ptr<D3D12RootSignature> m_rootSignature;
	std::unique_ptr<ShaderCompileResult> m_vertexShader;
	std::unique_ptr<ShaderCompileResult> m_pixelShader;
	std::unique_ptr<D3D12PipelineState> m_pipelineState;
};