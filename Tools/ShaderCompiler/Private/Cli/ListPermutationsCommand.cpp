#include "PCH.h"

#include "Cli/ListPermutationsCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>
#include <unordered_set>

static void PrintPermutationDomain(const ShaderRegistrationDesc& shader, const ShaderPermutationDomainDescriptor& domain)
{
	const std::vector<ShaderPermutationVector> vectors = EnumerateShaderPermutationVectors(domain);
	std::cout << GetShaderRegistrationPackageId(shader) << " shader=" << shader.ShaderName
	          << " permutationDimensions=" << domain.Dimensions.size()
	          << " permutations=" << vectors.size() << "\n";

	for (const ShaderPermutationDimensionDescriptor& dimension : domain.Dimensions)
	{
		std::cout << "  dimension name=" << dimension.Name
		          << " define=" << (dimension.DefineName.empty() ? "<none>" : dimension.DefineName)
		          << " values=";
		for (std::size_t valueIndex = 0; valueIndex < dimension.Values.size(); ++valueIndex)
		{
			if (valueIndex > 0)
			{
				std::cout << ",";
			}
			const ShaderPermutationValueDescriptor& value = dimension.Values[valueIndex];
			std::cout << valueIndex << ':' << value.Name << '=' << value.DefineValue;
		}
		std::cout << "\n";
	}

	for (const ShaderPermutationVector& vector : vectors)
	{
		const ShaderPermutationKey key = BuildShaderPermutationKey(domain, vector);
		const std::vector<std::string> defines = BuildShaderPermutationDefines(domain, vector);
		std::cout << "  variant=" << BuildShaderPermutationVariantId(key)
		          << " key=" << Formatting::FormatPrefixedHexUInt64(key)
		          << " vector=\"" << BuildShaderPermutationVectorName(domain, vector) << "\""
		          << " defines=";
		if (defines.empty())
		{
			std::cout << "<none>";
		}
		else
		{
			for (std::size_t defineIndex = 0; defineIndex < defines.size(); ++defineIndex)
			{
				if (defineIndex > 0)
				{
					std::cout << ",";
				}
				std::cout << defines[defineIndex];
			}
		}
		std::cout << "\n";
	}
}

int ListPermutationsCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() != 1)
	{
		std::cerr << "ShaderCompiler: list-permutations requires <shader-id>\n";
		return kExitCodeUsage;
	}

	bool foundTypedShader = false;
	std::unordered_set<std::string> printedPackageDomains;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string packageName = GetShaderRegistrationPackageId(shader);
		if (packageName != args[0] && shader.ShaderName != args[0])
		{
			continue;
		}

		foundTypedShader = true;
		if (packageName == args[0])
		{
			if (!printedPackageDomains.insert(packageName).second)
			{
				continue;
			}
		}

		const ShaderPermutationDomainDescriptor domain =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};
		if (domain.Dimensions.empty())
		{
			std::cout << packageName << " shader=" << shader.ShaderName << " permutation=Default variant=Default key=0x0000000000000000\n";
			continue;
		}

		PrintPermutationDomain(shader, domain);
	}

	if (foundTypedShader)
	{
		return kExitCodeSuccess;
	}

	std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
	return kExitCodeUsage;
}