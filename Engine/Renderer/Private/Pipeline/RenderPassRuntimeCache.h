#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/RenderPassShaderRuntime.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

class RenderDeviceServices;
class RenderHardwareInterface;

class RenderPassRuntimeCache final
{
private:
	template <typename TVertexShader, typename TPixelShader> struct GraphicsShaderPipelineKey;
	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeStorageHolder;

public:
	explicit RenderPassRuntimeCache(RenderDeviceServices& deviceServices);
	~RenderPassRuntimeCache() noexcept;

	RenderPassRuntimeCache(const RenderPassRuntimeCache&) = delete;
	RenderPassRuntimeCache& operator=(const RenderPassRuntimeCache&) = delete;
	RenderPassRuntimeCache(RenderPassRuntimeCache&&) = delete;
	RenderPassRuntimeCache& operator=(RenderPassRuntimeCache&&) = delete;

	std::uint64_t GetShaderGeneration() const noexcept;
	void ReloadShaders();
	void PollRetiredGenerations() noexcept;

	template <typename TShader> void MaterializeComputeShaderRuntime() const noexcept
	{
		RuntimeStorageHolder<TShader>& holder = GetOrCreateRuntimeStorageHolder<TShader>();
		if (!holder.Runtime.has_value())
		{
			try
			{
				holder.Create(*m_renderHardwareInterface, *m_activeGeneration);
			}
			catch (const Diagnostics::Error& error)
			{
				HandleRuntimeCreationFailure(error.what());
			}
		}
	}

	template <typename TShader> const ComputePassPipelineRuntime& GetComputeShaderRuntime() const noexcept
	{
		const RuntimeStorageHolder<TShader>* const holder = FindRuntimeStorageHolder<TShader>();
		assert(holder != nullptr && holder->Runtime.has_value() && "Shader runtime must be materialized before recording.");
		return *holder->Runtime;
	}

	template <typename TVertexShader, typename TPixelShader>
	void MaterializeGraphicsShaderRuntime(const GraphicsShaderPipelineState& pipelineState) const noexcept
	{
		GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>& holder =
		    GetOrCreateGraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>(pipelineState);
		if (!holder.Runtime.has_value())
		{
			try
			{
				holder.Create(*m_renderHardwareInterface, *m_activeGeneration);
			}
			catch (const Diagnostics::Error& error)
			{
				HandleRuntimeCreationFailure(error.what());
			}
		}
	}

	template <typename TVertexShader, typename TPixelShader> const RasterPassPipelineRuntime& GetGraphicsShaderRuntime() const noexcept
	{
		using PipelineKey = GraphicsShaderPipelineKey<TVertexShader, TPixelShader>;
		const auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		const auto runtime = holders.find(typeid(PipelineKey));
		assert(runtime != holders.end() && "Graphics shader runtime must be materialized before recording.");
		const auto* const holder = static_cast<const GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>*>(runtime->second.get());
		assert(holder != nullptr && holder->Runtime.has_value());
		return *holder->Runtime;
	}

private:
	struct ShaderRuntimeGeneration;
	struct IRuntimeStorageHolder
	{
		virtual ~IRuntimeStorageHolder() noexcept;
		virtual std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation) const = 0;
	};

	template <typename TShader> struct RuntimeStorageHolder final : IRuntimeStorageHolder
	{
		void Create(RenderHardwareInterface& renderHardwareInterface, const ShaderRuntimeGeneration& generation)
		{
			const ShaderRegistrationDesc& shader = GlobalShader<TShader>::GetRegistration();
			const ShaderRef<TShader> shaderRef = ShaderRef<TShader>::Resolve(generation.Map, generation.Library, generation.Target);
			RenderPassShaderRuntime::CreateComputeRuntime(
			    renderHardwareInterface,
			    shader,
			    shaderRef,
			    Storage,
			    [](ComputePipelineDesc&) {});
			Runtime.emplace(*Storage.BindingLayout, *Storage.Pipeline);
		}

		std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation) const override
		{
			auto replacement = std::make_unique<RuntimeStorageHolder<TShader>>();
			replacement->Create(renderHardwareInterface, generation);
			return replacement;
		}

		RenderPassShaderRuntimeStorage Storage;
		std::optional<ComputePassPipelineRuntime> Runtime;
	};

	template <typename TVertexShader, typename TPixelShader> struct GraphicsShaderPipelineKey final
	{
	};

	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeStorageHolder final : IRuntimeStorageHolder
	{
		explicit GraphicsRuntimeStorageHolder(GraphicsShaderPipelineState pipelineState) noexcept :
		    PipelineState(std::move(pipelineState))
		{
		}

		void Create(RenderHardwareInterface& renderHardwareInterface, const ShaderRuntimeGeneration& generation)
		{
			const ShaderRegistrationDesc& vertexShader = GlobalShader<TVertexShader>::GetRegistration();
			const ShaderRegistrationDesc& pixelShader = GlobalShader<TPixelShader>::GetRegistration();
			const ShaderRef<TVertexShader> vertexRef =
			    ShaderRef<TVertexShader>::Resolve(generation.Map, generation.Library, generation.Target);
			const ShaderRef<TPixelShader> pixelRef =
			    ShaderRef<TPixelShader>::Resolve(generation.Map, generation.Library, generation.Target);
			RenderPassShaderRuntime::CreateGraphicsRuntime(
			    renderHardwareInterface,
			    vertexShader,
			    vertexRef,
			    pixelShader,
			    pixelRef,
			    true,
			    Storage,
			    [this](GraphicsPipelineDesc& pipeline)
			    {
				    pipeline.VertexLayout = PipelineState.VertexLayout;
				    pipeline.RenderWireframe = PipelineState.RenderWireframe;
				    pipeline.CullMode = PipelineState.CullMode;
				    pipeline.FrontFaceWinding = PipelineState.FrontFaceWinding;
				    pipeline.DepthTest = PipelineState.DepthTest;
				    pipeline.StencilTest = PipelineState.StencilTest;
				    pipeline.RenderTargetFormats = PipelineState.RenderTargetFormats;
				    pipeline.RenderTargetCount = PipelineState.RenderTargetCount;
				    pipeline.DepthStencilFormat = PipelineState.DepthStencilFormat;
			    });
			Runtime.emplace(*Storage.BindingLayout, *Storage.Pipeline, Storage.WireframePipeline.get(), Storage.TwoSidedPipeline.get());
		}

		std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation) const override
		{
			auto replacement = std::make_unique<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>>(PipelineState);
			replacement->Create(renderHardwareInterface, generation);
			return replacement;
		}

		GraphicsShaderPipelineState PipelineState;
		RenderPassShaderRuntimeStorage Storage;
		std::optional<RasterPassPipelineRuntime> Runtime;
	};

	struct ShaderRuntimeGeneration final
	{
		std::uint64_t Generation = 1;
		ShaderTarget Target = kDefaultShaderTarget;
		CookedShaderLibrary Library;
		GlobalShaderMap Map;
		std::unordered_map<std::type_index, std::unique_ptr<IRuntimeStorageHolder>> RuntimeStorageByShaderType;
	};

	struct RetiredShaderRuntimeGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<ShaderRuntimeGeneration> Runtime;
	};

	template <typename TShader> RuntimeStorageHolder<TShader>& GetOrCreateRuntimeStorageHolder() const noexcept
	{
		const std::type_index shaderType = typeid(TShader);
		auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		auto runtime = holders.find(shaderType);
		if (runtime == holders.end())
		{
			runtime = holders.emplace(shaderType, std::make_unique<RuntimeStorageHolder<TShader>>()).first;
		}

		auto* typedHolder = static_cast<RuntimeStorageHolder<TShader>*>(runtime->second.get());
		assert(typedHolder != nullptr);
		return *typedHolder;
	}

	template <typename TShader> const RuntimeStorageHolder<TShader>* FindRuntimeStorageHolder() const noexcept
	{
		const auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		const auto runtime = holders.find(typeid(TShader));
		return runtime != holders.end() ? static_cast<const RuntimeStorageHolder<TShader>*>(runtime->second.get()) : nullptr;
	}

	template <typename TVertexShader, typename TPixelShader>
	GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>& GetOrCreateGraphicsRuntimeStorageHolder(
	    const GraphicsShaderPipelineState& pipelineState) const noexcept
	{
		using PipelineKey = GraphicsShaderPipelineKey<TVertexShader, TPixelShader>;
		auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		auto runtime = holders.find(typeid(PipelineKey));
		if (runtime == holders.end())
		{
			runtime = holders
			              .emplace(
			                  typeid(PipelineKey),
			                  std::make_unique<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>>(pipelineState))
			              .first;
		}
		auto* const holder = static_cast<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>*>(runtime->second.get());
		assert(holder != nullptr);
		if (!(holder->PipelineState == pipelineState))
		{
			HandleRuntimeCreationFailure("One graphics shader pair cannot silently reuse a different pipeline state.");
		}
		return *holder;
	}

	RhiSubmissionState CaptureLastSubmittedState() const noexcept;
	std::unique_ptr<ShaderRuntimeGeneration> OpenGeneration(std::uint64_t generation) const;
	static void ValidateGenerationContracts(const ShaderRuntimeGeneration& generation);
	bool IsComplete(const RhiSubmissionState& state) const noexcept;
	[[noreturn]] void HandleRuntimeCreationFailure(std::string_view errorMessage) const;

	RenderDeviceServices* m_deviceServices = nullptr;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	mutable std::unique_ptr<ShaderRuntimeGeneration> m_activeGeneration;
	std::vector<RetiredShaderRuntimeGeneration> m_retiredGenerations;
};
