#include "PCH.h"

#include "Cli/CommandRegistry.h"

#include "Cli/CookShadersCommand.h"
#include "Cli/InspectManifestCommand.h"
#include "Constants/ShaderCompilerConstants.h"

#include <algorithm>

CommandRegistry::CommandRegistry()
{
	m_registrations.push_back(Registration{
	    .verbs = {kCommandInspectManifest, kCommandInspectManifestLegacy},
	    .command = std::make_shared<InspectManifestCommand>(),
	    .usageLine = "  ShaderCompiler inspect-manifest",
	    .legacyUsageLine = "  ShaderCompiler inspect-shader-manifest"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandCook, kCommandCookLegacy},
	    .command = std::make_shared<CookShadersCommand>(),
	    .usageLine = "  ShaderCompiler cook [--no-cache] [--cache-dir <path>]",
	    .legacyUsageLine = "  ShaderCompiler cook-shaders"});
}

const ICommand* CommandRegistry::Find(std::string_view verb) const noexcept
	{
		for (const Registration& registration : m_registrations)
		{
			if (std::any_of(
			        registration.verbs.begin(),
			        registration.verbs.end(),
			        [verb](std::string_view candidate) { return candidate == verb; }))
			{
				return registration.command.get();
			}
		}
		return nullptr;
	}


void CommandRegistry::PrintUsage(std::ostream& output) const

	{
		output << "Usage:\n";
		for (const Registration& registration : m_registrations)
		{
			output << registration.usageLine << '\n';
		}
		output << "\nCompatibility:\n";
		for (const Registration& registration : m_registrations)
		{
			if (!registration.legacyUsageLine.empty())
			{
				output << registration.legacyUsageLine << '\n';
			}
		}
	}
