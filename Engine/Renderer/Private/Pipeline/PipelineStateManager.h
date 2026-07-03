#pragma once

#include "Pipeline/RenderPassDefinitionRuntime.h"
#include "Shaders/CookedShaderReloadResult.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <string_view>

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
		const std::string_view key = TPass::GetDefinition().PassName;
		auto runtimeIt = m_runtimeStorageByPassName.find(key);
		if (runtimeIt == m_runtimeStorageByPassName.end())
		{
			runtimeIt = m_runtimeStorageByPassName.emplace(key, std::make_unique<RuntimeStorageHolder<TPass>>()).first;
		}

		auto* typedHolder = static_cast<RuntimeStorageHolder<TPass>*>(runtimeIt->second.get());
		assert(typedHolder != nullptr);
		return *typedHolder;
	}

	[[noreturn]] void HandleRuntimeCreationFailure(const std::string& errorMessage) const;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	mutable CookedShaderPackageCache m_shaderPackages;
	mutable std::unordered_map<std::string_view, std::unique_ptr<IRuntimeStorageHolder>> m_runtimeStorageByPassName;
};
