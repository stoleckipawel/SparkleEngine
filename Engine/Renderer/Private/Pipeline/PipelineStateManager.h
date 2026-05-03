#pragma once

#include "Pipeline/RenderPassPipelineTraits.h"
#include "FrameGraph/RenderPassRuntime.h"
#include "Shaders/CookedShaderReloadResult.h"

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

class RenderHardwareInterface;

class PipelineStateManager final
{
  public:
	explicit PipelineStateManager(RenderHardwareInterface& rhi) noexcept;
	~PipelineStateManager() noexcept;

	PipelineStateManager(const PipelineStateManager&) = delete;
	PipelineStateManager& operator=(const PipelineStateManager&) = delete;
	PipelineStateManager(PipelineStateManager&&) = delete;
	PipelineStateManager& operator=(PipelineStateManager&&) = delete;

	std::uint64_t GetShaderPackageGeneration() const noexcept { return m_shaderPackages.GetGeneration(); }
	CookedShaderReloadResult ReloadCookedShaders() noexcept;

	template <typename TPass> const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		RuntimeStorageHolder<TPass>& holder = GetOrCreateRuntimeStorageHolder<TPass>();
		if (!holder.Runtime.has_value())
		{
			std::string errorMessage;
			if (!RenderPassPipelineTraits<TPass>::CreateRuntimeStorage(*m_rhi, m_shaderPackages, holder.Storage, errorMessage))
			{
				HandleRuntimeCreationFailure(errorMessage);
			}

			holder.Runtime.emplace(RenderPassPipelineTraits<TPass>::MakeRuntime(holder.Storage));
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
		RenderPassRuntimeStorage<TPass> Storage;
		std::optional<typename RenderPassRuntimeTraits<TPass>::RuntimeType> Runtime;
	};

	template <typename TPass> RuntimeStorageHolder<TPass>& GetOrCreateRuntimeStorageHolder() const noexcept
	{
		const std::type_index key = std::type_index(typeid(TPass));
		auto runtimeIt = m_runtimeStorageByPass.find(key);
		if (runtimeIt == m_runtimeStorageByPass.end())
		{
			runtimeIt = m_runtimeStorageByPass.emplace(key, std::make_unique<RuntimeStorageHolder<TPass>>()).first;
		}

		auto* typedHolder = static_cast<RuntimeStorageHolder<TPass>*>(runtimeIt->second.get());
		assert(typedHolder != nullptr);
		return *typedHolder;
	}

	[[noreturn]] void HandleRuntimeCreationFailure(const std::string& errorMessage) const;

	RenderHardwareInterface* m_rhi = nullptr;
	mutable CookedShaderPackageCache m_shaderPackages;
	mutable std::unordered_map<std::type_index, std::unique_ptr<IRuntimeStorageHolder>> m_runtimeStorageByPass;
};