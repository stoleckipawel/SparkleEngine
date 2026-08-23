#include "PCH.h"

#include "Cli/InspectShaderCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "RHI/Public/Shaders/GlobalShaderMap.h"

#include <iostream>

int InspectShaderCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() != 1)
	{
		std::cerr << "ShaderCompiler: inspect-shader requires <shader-id>\n";
		return kExitCodeUsage;
	}

	try
	{
		const ShaderContractCatalog catalog = ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::ShaderId, args[0]);
		const ShaderContract& shader = catalog.front();
		const CookedShaderLibrary library = CookedShaderLibrary::Open(Filesystem::GetCookedShaderLibraryPath());
		const GlobalShaderMap map = GlobalShaderMap::Open(Filesystem::GetGlobalShaderMapPath(), library);
		std::cout << shader.shaderName << " type=" << Formatting::FormatPrefixedHexUInt64(shader.shaderTypeId)
		          << " stage=" << GetShaderStagePrefix(shader.stage) << " source=" << shader.sourcePath
		          << " entry=" << shader.entryPoint << " parameters=" << shader.parameterStruct.Fields.size() << "\n";
		for (const ShaderTarget target : {ShaderTarget::DxilSm66, ShaderTarget::SpirV16})
		{
			const GlobalShaderMapEntry* const entry = map.Find(shader.shaderTypeId, target);
			if (entry == nullptr)
			{
				continue;
			}
			std::cout << "  target=" << GetShaderTargetName(target)
			          << " codeHash=" << Formatting::FormatPrefixedHexUInt64(entry->CodeHash)
			          << " parameterSignature=" << Formatting::FormatPrefixedHexUInt64(entry->ParameterSignature)
			          << " compileInputHash=" << Formatting::FormatPrefixedHexUInt64(entry->CompileInputHash) << "\n";
		}
		return kExitCodeSuccess;
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << "ShaderCompiler: inspect-shader failed - " << error.what() << "\n";
		return kExitCodeCookFailure;
	}
}
