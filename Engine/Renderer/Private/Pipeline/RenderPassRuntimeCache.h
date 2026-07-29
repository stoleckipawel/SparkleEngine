#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "Pipeline/RenderPassDefinitionRuntime.h"
#include "RHI/Public/Commands/RhiQueue.h"

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
  public:
	explicit RenderPassRuntimeCache(RenderDeviceServices& deviceServices) noexcept;
	~RenderPassRuntimeCache() noexcept;

	RenderPassRuntimeCache(const RenderPassRuntimeCache&) = delete;
	RenderPassRuntimeCache& operator=(const RenderPassRuntimeCache&) = delete;
	RenderPassRuntimeCache(RenderPassRuntimeCache&&) = delete;
	RenderPassRuntimeCache& operator=(RenderPassRuntimeCache&&) = delete;

	std::uint64_t GetShaderPackageGeneration() const noexcept;
	void ReloadCookedShaders();
	void PollRetiredGenerations() noexcept;

	template <typename TPass> void MaterializePassRuntime() const noexcept
	{
		RuntimeStorageHolder<TPass>& holder = GetOrCreateRuntimeStorageHolder<TPass>();
		if (!holder.Runtime.has_value())
		{
			try
			{
				holder.Create(*m_renderHardwareInterface, m_activeGeneration->ShaderPackages);
			}
			catch (const Diagnostics::Error& error)
			{
				HandleRuntimeCreationFailure(error.what());
			}
		}
	}

	template <typename TPass> const typename TPass::PipelineRuntime& GetPassRuntime() const noexcept
	{
		const RuntimeStorageHolder<TPass>* const holder = FindRuntimeStorageHolder<TPass>();
		assert(holder != nullptr && holder->Runtime.has_value() && "Pass runtime must be materialized before recording.");
		return *holder->Runtime;
	}

  private:
	struct IRuntimeStorageHolder
	{
		virtual ~IRuntimeStorageHolder() noexcept;
		virtual std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    CookedShaderPackageCache& shaderPackages) const = 0;
	};

	template <typename TPass>
	struct RuntimeStorageHolder final : IRuntimeStorageHolder
	{
		void Create(
		    RenderHardwareInterface& renderHardwareInterface,
		    CookedShaderPackageCache& shaderPackages)
		{
			RenderPassDefinitionRuntime::CreateRuntimeStorage(
			    renderHardwareInterface,
			    shaderPackages,
			    TPass::GetDefinition(),
			    Storage);
			Runtime.emplace(
			    RenderPassDefinitionRuntime::MakeRuntime<
			        typename TPass::PipelineRuntime>(Storage));
		}

		std::unique_ptr<IRuntimeStorageHolder> CreateReplacement(
		    RenderHardwareInterface& renderHardwareInterface,
		    CookedShaderPackageCache& shaderPackages) const override
		{
			auto replacement = std::make_unique<RuntimeStorageHolder<TPass>>();
			replacement->Create(renderHardwareInterface, shaderPackages);
			return replacement;
		}

		RenderPassShaderRuntimeStorage Storage;
		std::optional<typename TPass::PipelineRuntime> Runtime;
	};

	struct ShaderRuntimeGeneration final
	{
		std::uint64_t Generation = 1;
		CookedShaderPackageCache ShaderPackages;
		std::unordered_map<
		    std::type_index,
		    std::unique_ptr<IRuntimeStorageHolder>>
		    RuntimeStorageByPassType;
	};

	struct RetiredShaderRuntimeGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<ShaderRuntimeGeneration> Runtime;
	};

	template <typename TPass>
	RuntimeStorageHolder<TPass>& GetOrCreateRuntimeStorageHolder() const noexcept
	{
		const std::type_index passType = typeid(TPass);
		auto& holders = m_activeGeneration->RuntimeStorageByPassType;
		auto runtime = holders.find(passType);
		if (runtime == holders.end())
		{
			runtime =
			    holders
			        .emplace(
			            passType,
			            std::make_unique<RuntimeStorageHolder<TPass>>())
			        .first;
		}

		auto* typedHolder =
		    static_cast<RuntimeStorageHolder<TPass>*>(runtime->second.get());
		assert(typedHolder != nullptr);
		return *typedHolder;
	}

	template <typename TPass> const RuntimeStorageHolder<TPass>* FindRuntimeStorageHolder() const noexcept
	{
		const auto& holders = m_activeGeneration->RuntimeStorageByPassType;
		const auto runtime = holders.find(typeid(TPass));
		return runtime != holders.end() ? static_cast<const RuntimeStorageHolder<TPass>*>(runtime->second.get()) : nullptr;
	}

	RhiSubmissionState CaptureLastSubmittedState() const noexcept;
	bool IsComplete(const RhiSubmissionState& state) const noexcept;
	[[noreturn]] void HandleRuntimeCreationFailure(
	    std::string_view errorMessage) const;

	RenderDeviceServices* m_deviceServices = nullptr;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	mutable std::unique_ptr<ShaderRuntimeGeneration> m_activeGeneration;
	std::vector<RetiredShaderRuntimeGeneration> m_retiredGenerations;
};
