#include "../PCH.h"

#include "PipelineStateManager.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <algorithm>
#include <array>
#include <limits>

PipelineStateManager::PipelineStateManager(
    RenderDeviceServices& deviceServices) noexcept :
    m_deviceServices(&deviceServices),
    m_renderHardwareInterface(&deviceServices.GetRenderHardwareInterface()),
    m_activeGeneration(std::make_unique<ShaderRuntimeGeneration>())
{
}

PipelineStateManager::~PipelineStateManager() noexcept = default;

std::uint64_t PipelineStateManager::GetShaderPackageGeneration() const noexcept
{
	return m_activeGeneration != nullptr
	           ? m_activeGeneration->Generation
	           : 0;
}

CookedShaderReloadResult PipelineStateManager::ReloadCookedShaders() noexcept
{
	PollRetiredGenerations();
	if (m_activeGeneration == nullptr ||
	    m_renderHardwareInterface == nullptr)
	{
		return CookedShaderReloadResult::Failure(
		    "Shader runtime generation has no active renderer owner.");
	}

	auto replacement = std::make_unique<ShaderRuntimeGeneration>();
	replacement->Generation = m_activeGeneration->Generation + 1;
	if (replacement->Generation == 0)
	{
		return CookedShaderReloadResult::Failure(
		    "Shader runtime generation identity exhausted.");
	}

	for (const auto& [passType, activeHolder] :
	     m_activeGeneration->RuntimeStorageByPassType)
	{
		if (activeHolder == nullptr)
		{
			continue;
		}

		std::unique_ptr<IRuntimeStorageHolder> replacementHolder;
		std::string errorMessage;
		if (!activeHolder->TryCreateReplacement(
		        *m_renderHardwareInterface,
		        replacement->ShaderPackages,
		        replacementHolder,
		        errorMessage))
		{
			return CookedShaderReloadResult::Failure(
			    "Shader runtime replacement validation failed; active "
			    "generation remains unchanged. " +
			    errorMessage);
		}
		replacement->RuntimeStorageByPassType.emplace(
		    passType,
		    std::move(replacementHolder));
	}

	m_retiredGenerations.push_back(
	    RetiredShaderRuntimeGeneration{
	        .LastUse = CaptureLastSubmittedState(),
	        .Runtime = std::move(m_activeGeneration)});
	m_activeGeneration = std::move(replacement);
	return CookedShaderReloadResult::Success();
}

void PipelineStateManager::PollRetiredGenerations() noexcept
{
	m_retiredGenerations.erase(
	    std::remove_if(
	        m_retiredGenerations.begin(),
	        m_retiredGenerations.end(),
	        [this](
	            const RetiredShaderRuntimeGeneration& generation) noexcept
	        {
		        return IsComplete(generation.LastUse);
	        }),
	    m_retiredGenerations.end());
}

RhiSubmissionState PipelineStateManager::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	if (m_deviceServices == nullptr)
	{
		return state;
	}

	for (std::size_t queueIndex = 0;
	     queueIndex < RhiQueueTypeCount;
	     ++queueIndex)
	{
		state.MarkUsed(
		    m_deviceServices->GetLastSubmittedToken(
		        static_cast<ERhiQueueType>(queueIndex)));
	}
	return state;
}

bool PipelineStateManager::IsComplete(
    const RhiSubmissionState& state) const noexcept
{
	if (m_deviceServices == nullptr)
	{
		return true;
	}

	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = state.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0;
	     tokenIndex < tokenCount;
	     ++tokenIndex)
	{
		if (!m_deviceServices->IsSubmissionComplete(tokens[tokenIndex]))
		{
			return false;
		}
	}
	return true;
}

[[noreturn]] void PipelineStateManager::HandleRuntimeCreationFailure(
    const std::string& errorMessage) const
{
	Diagnostics::Fail(
	    Logging::GetOrCreateLogger("Renderer"),
	    __FILE__,
	    __LINE__,
	    errorMessage);
}
