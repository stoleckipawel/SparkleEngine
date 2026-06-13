#include "PCH.h"

#include "Cli/ListShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace
{
	struct ShaderPackageValidationState final
	{
		std::string BindingLayoutId;
		CookedShaderPackageKind PackageKind = CookedShaderPackageKind::Graphics;
		std::unordered_set<std::uint8_t> DeclaredStages;
	};

	int ValidateShaderRegistrations(std::span<const ShaderRegistrationDesc> typedShaders)
	{
		std::unordered_set<std::string> shaderNames;
		std::unordered_map<std::string, ShaderPackageValidationState> packages;
		int errorCount = 0;

		for (const ShaderRegistrationDesc& shader : typedShaders)
		{
			const std::string shaderName(shader.ShaderName);
			const std::string packageId = GetShaderRegistrationPackageId(shader);
			const std::string bindingLayoutId = GetShaderRegistrationBindingLayoutId(shader);

			const auto reportError = [&](std::string_view reason)
			{
				std::cerr << "ShaderCompiler: shader registration invalid"
				          << " shader=" << (shaderName.empty() ? "<empty>" : shaderName)
				          << " package=" << (packageId.empty() ? "<empty>" : packageId)
				          << " layout=" << (bindingLayoutId.empty() ? "<empty>" : bindingLayoutId)
				          << " source=" << (shader.SourcePath.empty() ? "<empty>" : shader.SourcePath)
				          << " entry=" << (shader.EntryPoint.empty() ? "<empty>" : shader.EntryPoint)
				          << " stage=" << GetShaderStagePrefix(shader.Stage)
				          << " reason=" << reason << "\n";
				++errorCount;
			};

			if (shaderName.empty())
			{
				reportError("empty-shader-name");
			}
			else if (!shaderNames.insert(shaderName).second)
			{
				reportError("duplicate-shader-name");
			}

			if (packageId.empty())
			{
				reportError("empty-package-id");
			}
			if (bindingLayoutId.empty())
			{
				reportError("empty-binding-layout-id");
			}
			if (shader.SourcePath.empty())
			{
				reportError("empty-source-path");
			}
			if (shader.EntryPoint.empty())
			{
				reportError("empty-entry-point");
			}
			if (shader.BuildParameterStructDescriptor == nullptr)
			{
				reportError("missing-parameter-descriptor-builder");
			}

			const bool rayTracingLibrary = shader.PackageKind == CookedShaderPackageKind::RayTracingLibrary;
			if (shader.Stage == ShaderStage::Count && !rayTracingLibrary)
			{
				reportError("stage-count-is-only-valid-for-ray-tracing-library-packages");
			}
			if (shader.Stage != ShaderStage::Count && rayTracingLibrary)
			{
				reportError("ray-tracing-library-package-must-use-library-stage");
			}

			ShaderPackageValidationState& packageState = packages[packageId];
			if (packageState.BindingLayoutId.empty())
			{
				packageState.BindingLayoutId = bindingLayoutId;
				packageState.PackageKind = shader.PackageKind;
			}
			else
			{
				if (packageState.BindingLayoutId != bindingLayoutId)
				{
					reportError("package-binding-layout-mismatch");
				}
				if (packageState.PackageKind != shader.PackageKind)
				{
					reportError("package-kind-mismatch");
				}
			}

			if (!rayTracingLibrary)
			{
				const std::uint8_t stageKey = static_cast<std::uint8_t>(shader.Stage);
				if (!packageState.DeclaredStages.insert(stageKey).second)
				{
					reportError("duplicate-stage-in-package");
				}
			}
		}

		return errorCount;
	}
}

int ListShadersCommand::Run(std::span<const std::string_view> args) const
{
	const bool validateOnly = args.size() == 1 && args[0] == "--validate";
	if (!args.empty() && !validateOnly)
	{
		std::cerr << "ShaderCompiler: list-shaders accepts only optional --validate\n";
		return kExitCodeUsage;
	}

	const std::span<const ShaderRegistrationDesc> typedShaders =
	    GlobalShaderRegistry::GetRegistrations();

	if (validateOnly)
	{
		const int errorCount = ValidateShaderRegistrations(typedShaders);
		if (errorCount > 0)
		{
			std::cerr << "ShaderCompiler: " << errorCount << " shader registration validation error(s)\n";
			return kExitCodeCookFailure;
		}
		std::cout << "ShaderCompiler: " << typedShaders.size() << " typed shader registration(s) valid\n";
		return kExitCodeSuccess;
	}

	if (!typedShaders.empty())
	{
		std::cout << "Typed shader registrations:\n";
		for (const ShaderRegistrationDesc& shader : typedShaders)
		{
			const ShaderParameterStructDescriptor parameters =
			    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};

			std::cout << shader.ShaderName << " package=" << GetShaderRegistrationPackageId(shader)
			          << " layout=" << GetShaderRegistrationBindingLayoutId(shader)
			          << " stage=" << GetShaderStagePrefix(shader.Stage)
			          << " source=" << shader.SourcePath
			          << " entry=" << shader.EntryPoint
			          << " parameters=" << parameters.Fields.size() << "\n";
		}
	}
	return kExitCodeSuccess;
}
