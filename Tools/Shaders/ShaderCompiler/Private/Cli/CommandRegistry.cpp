#include "PCH.h"

#include "Cli/CommandRegistry.h"

#include "Cli/CookShadersCommand.h"
#include "Cli/InspectPackageCommand.h"
#include "Cli/InspectShaderCommand.h"
#include "Cli/ListBackendsCommand.h"
#include "Cli/ListShadersCommand.h"
#include "Cli/ListTargetsCommand.h"
#include "Constants/ShaderCompilerConstants.h"

#include <algorithm>

CommandRegistry::CommandRegistry()
{
	m_registrations.push_back(Registration{
	    .verbs = {kCommandCook},
	    .command = std::make_shared<CookShadersCommand>(),
	    .usageLine = "  ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [--target <name>] [--backend <name>] [--debug-artifacts <dir>] [--analysis <pass>] [--debug-info] [--disable-optimizations]"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandListBackends},
	    .command = std::make_shared<ListBackendsCommand>(),
	    .usageLine = "  ShaderCompiler list-backends"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandListTargets},
	    .command = std::make_shared<ListTargetsCommand>(),
	    .usageLine = "  ShaderCompiler list-targets"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandInspectPackage},
	    .command = std::make_shared<InspectPackageCommand>(),
	    .usageLine = "  ShaderCompiler inspect-package <path>"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandListShaders},
	    .command = std::make_shared<ListShadersCommand>(),
	    .usageLine = "  ShaderCompiler list-shaders [--validate]"});

	m_registrations.push_back(Registration{
	    .verbs = {kCommandInspectShader},
	    .command = std::make_shared<InspectShaderCommand>(),
	    .usageLine = "  ShaderCompiler inspect-shader <shader-id>"});
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

}
