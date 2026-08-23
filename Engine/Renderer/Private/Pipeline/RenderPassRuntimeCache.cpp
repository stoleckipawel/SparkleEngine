#include "../PCH.h"

#include "RenderPassRuntimeCache.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/FileSystemUtils.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"

#include <algorithm>
#include <array>
#include <format>
#include <unordered_set>

RenderPassRuntimeCache::IRuntimeStorageHolder::~IRuntimeStorageHolder() noexcept = default;

RenderPassRuntimeCache::RenderPassRuntimeCache(RenderDeviceServices& deviceServices) :
    m_deviceServices(&deviceServices),
    m_renderHardwareInterface(&deviceServices.GetRenderHardwareInterface()),
    m_activeGeneration(OpenGeneration(1))
{
}

RenderPassRuntimeCache::~RenderPassRuntimeCache() noexcept = default;

std::uint64_t RenderPassRuntimeCache::GetShaderGeneration() const noexcept
{
	return m_activeGeneration != nullptr ? m_activeGeneration->Generation : 0;
}

void RenderPassRuntimeCache::ReloadShaders()
{
	PollRetiredGenerations();
	if (m_activeGeneration == nullptr || m_renderHardwareInterface == nullptr)
	{
		HandleRuntimeCreationFailure("Shader runtime generation has no active renderer owner.");
	}

	const std::uint64_t replacementGeneration = m_activeGeneration->Generation + 1;
	if (replacementGeneration == 0)
	{
		HandleRuntimeCreationFailure("Shader runtime generation identity exhausted.");
	}

	try
	{
		auto replacement = OpenGeneration(replacementGeneration);
		for (const auto& [shaderType, activeHolder] : m_activeGeneration->RuntimeStorageByShaderType)
		{
			if (activeHolder == nullptr)
			{
				HandleRuntimeCreationFailure("Shader runtime cache contains an empty shader holder.");
			}

			replacement->RuntimeStorageByShaderType.emplace(
			    shaderType,
			    activeHolder->CreateReplacement(*m_renderHardwareInterface, *replacement));
		}
		m_retiredGenerations.push_back(
		    RetiredShaderRuntimeGeneration{.LastUse = CaptureLastSubmittedState(), .Runtime = std::move(m_activeGeneration)});
		m_activeGeneration = std::move(replacement);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(
		    std::string("Shader runtime replacement validation failed; active generation remains unchanged. ") + error.what());
	}

}

std::unique_ptr<RenderPassRuntimeCache::ShaderRuntimeGeneration> RenderPassRuntimeCache::OpenGeneration(std::uint64_t generation) const
{
	auto result = std::make_unique<ShaderRuntimeGeneration>();
	result->Generation = generation;
	result->Target = GetRuntimeShaderTarget(m_renderHardwareInterface->GetCapabilities().RuntimeShaderBinaryFormat);
	result->Library = CookedShaderLibrary::Open(Filesystem::GetCookedShaderLibraryPath());
	result->Map = GlobalShaderMap::Open(Filesystem::GetGlobalShaderMapPath(), result->Library);
	ValidateGenerationContracts(*result);
	return result;
}

void RenderPassRuntimeCache::ValidateGenerationContracts(const ShaderRuntimeGeneration& generation)
{
	std::unordered_set<ShaderTypeId> registeredShaderTypes;
	for (const ShaderRegistrationDesc& registration : GlobalShaderRegistry::GetRegistrations())
	{
		if (registration.TypeId == 0 || registration.BuildParameterStructDescriptor == nullptr
		    || !registeredShaderTypes.insert(registration.TypeId).second)
		{
			throw Diagnostics::Error("Renderer shader registration catalog contains an invalid or duplicate shader type.");
		}

		const GlobalShaderMapEntry* const entry = generation.Map.Find(registration.TypeId, generation.Target);
		if (entry == nullptr)
		{
			throw Diagnostics::Error(
			    std::format("Shader '{}' is missing from the active-target global shader map.", registration.ShaderName));
		}

		const PassParameterLayout parameterLayout = BuildShaderParameterLayout(registration);
		if (generation.Map.ResolveString(entry->ShaderName) != registration.ShaderName
		    || generation.Map.ResolveString(entry->EntryPoint) != registration.EntryPoint || entry->Stage != registration.Stage
		    || entry->Features != registration.Features
		    || entry->ParameterSignature != BuildShaderParameterSignature(parameterLayout))
		{
			throw Diagnostics::Error(
			    std::format("Shader '{}' map entry does not match its registered contract.", registration.ShaderName));
		}
	}
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
