#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/GraphicsPipelineMaterialization.h"
#include "Pipeline/RenderPassShaderRuntime.h"
#include "Pipeline/RayTracingPipelineRuntime.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"

#include <algorithm>
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
class RayTracingShaderTablePlan;

class RenderPassRuntimeCache final
{
private:
	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeTypeTag;
	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeStorageHolder;
	template <typename TRayGenerationShader> struct RayTracingRuntimeTypeTag;
	template <typename TRayGenerationShader> struct RayTracingRuntimeStorageHolder;

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
	void MaterializeGraphicsShaderRuntime(const GraphicsPipelineRequest& request) const noexcept
	{
		GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>& holder =
		    GetOrCreateGraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>();
		try
		{
			holder.Materialize(*m_renderHardwareInterface, *m_activeGeneration, request);
		}
		catch (const Diagnostics::Error& error)
		{
			HandleRuntimeCreationFailure(error.what());
		}
	}

	template <typename TVertexShader, typename TPixelShader>
	RasterPassRuntime GetGraphicsShaderRuntime(const GraphicsPipelineRequest& request) const noexcept
	{
		using RuntimeType = GraphicsRuntimeTypeTag<TVertexShader, TPixelShader>;
		const auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		const auto runtime = holders.find(typeid(RuntimeType));
		if (runtime == holders.end())
		{
			HandleRuntimeCreationFailure("Graphics shader runtime lookup preceded materialization.");
		}
		const auto* const holder = static_cast<const GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>*>(runtime->second.get());
		const RenderBindingLayout* const bindingLayout = holder != nullptr ? holder->GetBindingLayout() : nullptr;
		const RenderPipeline* const pipeline = holder != nullptr ? holder->FindPipeline(request) : nullptr;
		if (bindingLayout == nullptr || pipeline == nullptr)
		{
			HandleRuntimeCreationFailure("Graphics pipeline materialization did not publish the requested key.");
		}
		return RasterPassRuntime{*bindingLayout, *pipeline};
	}

	template <typename TRayGenerationShader>
	void MaterializeRayTracingRuntime(const RayTracingPipelineComposition& composition) const noexcept
	{
		RayTracingRuntimeStorageHolder<TRayGenerationShader>& holder = GetOrCreateRayTracingRuntimeStorageHolder<TRayGenerationShader>();
		if (holder.Find(composition) == nullptr)
		{
			try
			{
				holder.Materialize(*m_renderHardwareInterface, *m_activeGeneration, composition);
			}
			catch (const Diagnostics::Error& error)
			{
				HandleRuntimeCreationFailure(error.what());
			}
		}
	}

	template <typename TRayGenerationShader>
	RayTracingPassPipelineRuntime GetRayTracingRuntime(const RayTracingPipelineComposition& composition) const noexcept
	{
		using RuntimeType = RayTracingRuntimeTypeTag<TRayGenerationShader>;
		const auto storage = m_activeGeneration->RuntimeStorageByShaderType.find(typeid(RuntimeType));
		if (storage == m_activeGeneration->RuntimeStorageByShaderType.end())
		{
			HandleRuntimeCreationFailure("Ray-tracing runtime lookup preceded materialization.");
		}
		const auto* holder = static_cast<const RayTracingRuntimeStorageHolder<TRayGenerationShader>*>(storage->second.get());
		const RayTracingPipelineRuntime* runtime = holder != nullptr ? holder->Find(composition) : nullptr;
		if (runtime == nullptr)
		{
			HandleRuntimeCreationFailure("Ray-tracing composition lookup preceded exact materialization.");
		}
		return RayTracingPassPipelineRuntime{
		    .BindingLayout = runtime->GetBindingLayout(),
		    .Pipeline = runtime->GetPipeline(),
		    .Generation = runtime->GetGeneration()};
	}

	template <typename TRayGenerationShader>
	std::unique_ptr<RayTracingShaderTable> CreateRayTracingShaderTable(const RayTracingPipelineComposition& composition) const noexcept
	{
		using RuntimeType = RayTracingRuntimeTypeTag<TRayGenerationShader>;
		const auto storage = m_activeGeneration->RuntimeStorageByShaderType.find(typeid(RuntimeType));
		if (storage == m_activeGeneration->RuntimeStorageByShaderType.end())
		{
			HandleRuntimeCreationFailure("Ray-tracing shader-table creation preceded pipeline materialization.");
		}
		const auto* holder = static_cast<const RayTracingRuntimeStorageHolder<TRayGenerationShader>*>(storage->second.get());
		const RayTracingPipelineRuntime* runtime = holder != nullptr ? holder->Find(composition) : nullptr;
		if (runtime == nullptr)
		{
			HandleRuntimeCreationFailure("Ray-tracing shader-table creation has no exact pipeline composition.");
		}
		try
		{
			return runtime->CreateShaderTable(*m_renderHardwareInterface, composition);
		}
		catch (const Diagnostics::Error& error)
		{
			HandleRuntimeCreationFailure(error.what());
		}
	}

	template <typename TRayGenerationShader> std::unique_ptr<RayTracingShaderTable> CreateRayTracingShaderTable(
	    const RayTracingPipelineComposition& composition,
	    const RayTracingShaderTablePlan& plan) const noexcept
	{
		using RuntimeType = RayTracingRuntimeTypeTag<TRayGenerationShader>;
		const auto storage = m_activeGeneration->RuntimeStorageByShaderType.find(typeid(RuntimeType));
		if (storage == m_activeGeneration->RuntimeStorageByShaderType.end())
		{
			HandleRuntimeCreationFailure("Ray-tracing shader-table creation preceded pipeline materialization.");
		}
		const auto* holder = static_cast<const RayTracingRuntimeStorageHolder<TRayGenerationShader>*>(storage->second.get());
		const RayTracingPipelineRuntime* runtime = holder != nullptr ? holder->Find(composition) : nullptr;
		if (runtime == nullptr)
		{
			HandleRuntimeCreationFailure("Ray-tracing shader-table creation has no exact pipeline composition.");
		}
		try
		{
			return runtime->CreateShaderTable(*m_renderHardwareInterface, composition, plan);
		}
		catch (const Diagnostics::Error& error)
		{
			HandleRuntimeCreationFailure(error.what());
		}
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
			    Pipeline,
			    [](ComputePipelineDesc&) {});
			Runtime.emplace(*Storage.BindingLayout, *Pipeline);
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
		std::unique_ptr<RenderPipeline> Pipeline;
		std::optional<ComputePassPipelineRuntime> Runtime;
	};

	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeTypeTag final
	{
	};

	template <typename TVertexShader, typename TPixelShader> struct GraphicsRuntimeStorageHolder final : IRuntimeStorageHolder
	{
		void Create(RenderHardwareInterface& renderHardwareInterface, const ShaderRuntimeGeneration& generation)
		{
			Generation = generation.Generation;
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
			    Storage);
		}

		void Materialize(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation,
		    const GraphicsPipelineRequest& request)
		{
			if (Storage.BindingLayout == nullptr)
			{
				Create(renderHardwareInterface, generation);
			}
			const GraphicsPipelineKey key = BuildKey(request);
			if (Pipelines.contains(key))
			{
				return;
			}
			MaterializePipeline(renderHardwareInterface, request);
		}

		const RenderBindingLayout* GetBindingLayout() const noexcept { return Storage.BindingLayout.get(); }

		const RenderPipeline* FindPipeline(const GraphicsPipelineRequest& request) const noexcept
		{
			const auto pipeline = Pipelines.find(BuildKey(request));
			return pipeline != Pipelines.end() ? pipeline->second.get() : nullptr;
		}

		std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation) const override
		{
			auto replacement = std::make_unique<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>>();
			replacement->Create(renderHardwareInterface, generation);
			std::vector<const GraphicsPipelineKey*> requestedKeys;
			requestedKeys.reserve(Pipelines.size());
			for (const auto& entry : Pipelines)
			{
				requestedKeys.push_back(&entry.first);
			}
			std::ranges::sort(
			    requestedKeys,
			    [](const GraphicsPipelineKey* left, const GraphicsPipelineKey* right)
			    { return GraphicsPipelineKeyHash{}(*left) < GraphicsPipelineKeyHash{}(*right); });
			for (const GraphicsPipelineKey* key : requestedKeys)
			{
				replacement->MaterializePipeline(renderHardwareInterface, key->Request);
			}
			return replacement;
		}

		GraphicsPipelineKey BuildKey(const GraphicsPipelineRequest& request) const noexcept
		{
			assert(Storage.Shaders.size() == 2 && Storage.Shaders[0].Entry != nullptr && Storage.Shaders[1].Entry != nullptr);
			return GraphicsPipelineKey{
			    .ShaderGeneration = Generation,
			    .VertexShaderCode = Storage.Shaders[0].Entry->CodeHash,
			    .PixelShaderCode = Storage.Shaders[1].Entry->CodeHash,
			    .BindingLayout = BuildShaderParameterSignature(Storage.ParameterLayout),
			    .Request = request};
		}

		void MaterializePipeline(RenderHardwareInterface& renderHardwareInterface, const GraphicsPipelineRequest& request)
		{
			const GraphicsPipelineKey key = BuildKey(request);
			const std::size_t keyHash = GraphicsPipelineKeyHash{}(key);
			const std::size_t bucket = Pipelines.bucket(key);
			for (auto existing = Pipelines.begin(bucket); existing != Pipelines.end(bucket); ++existing)
			{
				if (GraphicsPipelineKeyHash{}(existing->first) == keyHash && existing->first != key)
				{
					throw Diagnostics::Error("Distinct graphics pipeline keys produced the same hash.");
				}
			}
			std::wstring debugName = Strings::ToWide(GlobalShader<TVertexShader>::GetRegistration().ShaderName);
			debugName += L"_GraphicsKey_";
			debugName += std::to_wstring(keyHash);
			const GraphicsPipelineDesc desc =
			    BuildGraphicsPipelineDesc(request, *Storage.BindingLayout, Storage.Shaders[0], Storage.Shaders[1], debugName.c_str());
			Pipelines.emplace(key, PipelineRuntimeLibrary::CreateGraphicsPipeline(renderHardwareInterface, desc));
		}

		RenderPassShaderRuntimeStorage Storage;
		std::uint64_t Generation = 0;
		std::unordered_map<GraphicsPipelineKey, std::unique_ptr<RenderPipeline>, GraphicsPipelineKeyHash> Pipelines;
	};

	template <typename TRayGenerationShader> struct RayTracingRuntimeTypeTag final
	{
	};

	template <typename TRayGenerationShader> struct RayTracingRuntimeStorageHolder final : IRuntimeStorageHolder
	{
		struct Entry final
		{
			RayTracingPipelineComposition Definition;
			std::unique_ptr<RayTracingPipelineRuntime> Runtime;
		};

		void Materialize(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation,
		    const RayTracingPipelineComposition& composition)
		{
			if (composition.GetRayGeneration() != GlobalShader<TRayGenerationShader>::GetRegistration().TypeId)
			{
				throw Diagnostics::Error("Ray-tracing composition ray-generation type does not match its runtime key.");
			}
			Entries.push_back(
			    Entry{
			        .Definition = composition,
			        .Runtime = RayTracingPipelineRuntime::Create(
			            renderHardwareInterface,
			            generation.Map,
			            generation.Library,
			            generation.Target,
			            generation.Generation,
			            composition)});
		}

		const RayTracingPipelineRuntime* Find(const RayTracingPipelineComposition& composition) const noexcept
		{
			const auto entry = std::ranges::find(Entries, composition, &Entry::Definition);
			return entry != Entries.end() ? entry->Runtime.get() : nullptr;
		}

		std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    const ShaderRuntimeGeneration& generation) const override
		{
			auto replacement = std::make_unique<RayTracingRuntimeStorageHolder<TRayGenerationShader>>();
			for (const Entry& entry : Entries)
			{
				replacement->Materialize(renderHardwareInterface, generation, entry.Definition);
			}
			return replacement;
		}

		std::vector<Entry> Entries;
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
	GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>& GetOrCreateGraphicsRuntimeStorageHolder() const noexcept
	{
		using RuntimeType = GraphicsRuntimeTypeTag<TVertexShader, TPixelShader>;
		auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		auto runtime = holders.find(typeid(RuntimeType));
		if (runtime == holders.end())
		{
			runtime =
			    holders.emplace(typeid(RuntimeType), std::make_unique<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>>()).first;
		}
		auto* const holder = static_cast<GraphicsRuntimeStorageHolder<TVertexShader, TPixelShader>*>(runtime->second.get());
		assert(holder != nullptr);
		return *holder;
	}

	template <typename TRayGenerationShader>
	RayTracingRuntimeStorageHolder<TRayGenerationShader>& GetOrCreateRayTracingRuntimeStorageHolder() const noexcept
	{
		using RuntimeType = RayTracingRuntimeTypeTag<TRayGenerationShader>;
		auto& holders = m_activeGeneration->RuntimeStorageByShaderType;
		auto runtime = holders.find(typeid(RuntimeType));
		if (runtime == holders.end())
		{
			runtime = holders.emplace(typeid(RuntimeType), std::make_unique<RayTracingRuntimeStorageHolder<TRayGenerationShader>>()).first;
		}
		auto* holder = static_cast<RayTracingRuntimeStorageHolder<TRayGenerationShader>*>(runtime->second.get());
		assert(holder != nullptr);
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
