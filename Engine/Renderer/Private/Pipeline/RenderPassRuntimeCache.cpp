#include "../PCH.h"

#include "RenderPassRuntimeCache.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <algorithm>
#include <array>
#include <limits>

RenderPassRuntimeCache::IRuntimeStorageHolder::~IRuntimeStorageHolder() noexcept = default;

RenderPassRuntimeCache::RenderPassRuntimeCache(RenderDeviceServices& deviceServices) noexcept :
    m_deviceServices(&deviceServices),
    m_renderHardwareInterface(&deviceServices.GetRenderHardwareInterface()),
    m_activeGeneration(std::make_unique<ShaderRuntimeGeneration>())
{
}

RenderPassRuntimeCache::~RenderPassRuntimeCache() noexcept = default;

std::uint64_t RenderPassRuntimeCache::GetShaderPackageGeneration() const noexcept
{
	return m_activeGeneration != nullptr ? m_activeGeneration->Generation : 0;
}

void RenderPassRuntimeCache::ReloadCookedShaders()
{
	PollRetiredGenerations();
	if (m_activeGeneration == nullptr || m_renderHardwareInterface == nullptr)
	{
		HandleRuntimeCreationFailure("Shader runtime generation has no active renderer owner.");
	}

	auto replacement = std::make_unique<ShaderRuntimeGeneration>();
	replacement->Generation = m_activeGeneration->Generation + 1;
	if (replacement->Generation == 0)
	{
		HandleRuntimeCreationFailure("Shader runtime generation identity exhausted.");
	}

	try
	{
		for (const auto& [shaderType, activeHolder] : m_activeGeneration->RuntimeStorageByShaderType)
		{
			if (activeHolder == nullptr)
			{
				HandleRuntimeCreationFailure("Shader runtime cache contains an empty shader holder.");
			}

			replacement->RuntimeStorageByShaderType.emplace(
			    shaderType,
			    activeHolder->CreateReplacement(*m_renderHardwareInterface, replacement->ShaderPackages));
		}
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(
		    std::string("Shader runtime replacement validation failed; active generation remains unchanged. ") + error.what());
	}

	m_retiredGenerations.push_back(
	    RetiredShaderRuntimeGeneration{.LastUse = CaptureLastSubmittedState(), .Runtime = std::move(m_activeGeneration)});
	m_activeGeneration = std::move(replacement);
}

void RenderPassRuntimeCache::PollRetiredGenerations() noexcept
{
	m_retiredGenerations.erase(
	    std::remove_if(
	        m_retiredGenerations.begin(),
	        m_retiredGenerations.end(),
	        [this](const RetiredShaderRuntimeGeneration& generation) noexcept { return IsComplete(generation.LastUse); }),
	    m_retiredGenerations.end());
}

RhiSubmissionState RenderPassRuntimeCache::CaptureLastSubmittedState() const noexcept
{
	RhiSubmissionState state;
	if (m_deviceServices == nullptr)
	{
		return state;
	}

	for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
	{
		state.MarkUsed(m_deviceServices->GetLastSubmittedToken(static_cast<ERhiQueueType>(queueIndex)));
	}
	return state;
}

bool RenderPassRuntimeCache::IsComplete(const RhiSubmissionState& state) const noexcept
{
	if (m_deviceServices == nullptr)
	{
		return true;
	}

	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = state.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
	{
		if (!m_deviceServices->IsSubmissionComplete(tokens[tokenIndex]))
		{
			return false;
		}
	}
	return true;
}

[[noreturn]] void RenderPassRuntimeCache::HandleRuntimeCreationFailure(std::string_view errorMessage) const
{
	Diagnostics::Fatal(Logging::GetOrCreateLogger("Renderer"), __FILE__, __LINE__, errorMessage);
}
