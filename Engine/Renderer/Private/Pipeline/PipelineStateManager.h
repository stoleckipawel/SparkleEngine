#pragma once

#include "Pipeline/RenderPassDefinitionRuntime.h"
#include "Shaders/CookedShaderReloadResult.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

class RenderHardwareInterface;

class PipelineStateManager final
{
  public:
	explicit PipelineStateManager(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~PipelineStateManager() noexcept;

	PipelineStateManager(const PipelineStateManager&) = delete;
	PipelineStateManager& operator=(const PipelineStateManager&) = delete;
	PipelineStateManager(PipelineStateManager&&) = delete;
	PipelineStateManager& operator=(PipelineStateManager&&) = delete;

	std::uint64_t GetShaderPackageGeneration() const noexcept { return m_shaderPackages.GetGeneration(); }
	CookedShaderReloadResult ReloadCookedShaders() noexcept;

	template <typename TPass> const typename TPass::PipelineRuntime& GetPassRuntime() const noexcept
	{
		RuntimeStorageHolder<TPass>& holder = GetOrCreateRuntimeStorageHolder<TPass>();
		if (!holder.Runtime.has_value())
		{
			std::string errorMessage;
			if (!RenderPassDefinitionRuntime::TryCreateRuntimeStorage(
			        *m_renderHardwareInterface,
			        m_shaderPackages,
			        TPass::GetDefinition(),
			        holder.Storage,
			        errorMessage))
			{
				HandleRuntimeCreationFailure(errorMessage);
			}

			holder.Runtime.emplace(RenderPassDefinitionRuntime::MakeRuntime<typename TPass::PipelineRuntime>(holder.Storage));
		}

		return *holder.Runtime;
	}

  private:
	struct IRuntimeStorageHolder
	{
		virtual ~IRuntimeStorageHolder() noexcept = default;
	};

	template <typename TPass> struct RuntimeStorageHolder final : IRuntimeStorageHolder
	{
		RenderPassShaderRuntimeStorage Storage;
		std::optional<typename TPass::PipelineRuntime> Runtime;
	};

	template <typename TPass> RuntimeStorageHolder<TPass>& GetOrCreateRuntimeStorageHolder() const noexcept
	{
		const std::type_index passType = typeid(TPass);
		auto runtimeIt = m_runtimeStorageByPassType.find(passType);
		if (runtimeIt == m_runtimeStorageByPassType.end())
		{
			runtimeIt = m_runtimeStorageByPassType.emplace(passType, std::make_unique<RuntimeStorageHolder<TPass>>()).first;
		}

		auto* typedHolder = static_cast<RuntimeStorageHolder<TPass>*>(runtimeIt->second.get());
		assert(typedHolder != nullptr);
		return *typedHolder;
	}

	[[noreturn]] void HandleRuntimeCreationFailure(const std::string& errorMessage) const;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	mutable CookedShaderPackageCache m_shaderPackages;
	mutable std::unordered_map<std::type_index, std::unique_ptr<IRuntimeStorageHolder>> m_runtimeStorageByPassType;
};
