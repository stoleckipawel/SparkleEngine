#include "PCH.h"

#include "Cooking/Identity/ShaderCompileRequestHasher.h"

#include "Compiler/ShaderCompileProfile.h"
#include "Core/Public/Hash/HashUtils.h"

std::uint64_t ShaderCompileRequestHasher::Compute(const ShaderCompileRequest& request)
{
	std::string canonical;
	canonical.reserve(256);
	const auto appendString = [&canonical](std::string_view value)
	{
		canonical += std::to_string(value.size());
		canonical += ':';
		canonical += value;
		canonical += ';';
	};
	appendString("Sparkle.ShaderCompileRequest");
	appendString(request.VirtualSourcePath);
	appendString(ShaderCompileProfile::BuildTargetProfile(request));
	appendString(request.EntryPoint);
	if (IsSpirVTarget(request.Target))
	{
		appendString("StorageImageFormatUnknown");
		canonical += ShaderCompileProfile::SpirVUseUnknownStorageImageFormat ? '1' : '0';
		appendString("DirectXBufferLayout");
		canonical += ShaderCompileProfile::SpirVUseDirectXBufferLayout ? '1' : '0';
		appendString("DescriptorBindings=Canonical");
	}
	canonical += std::to_string(static_cast<std::uint32_t>(request.Stage));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(request.Target));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(request.UnitKind));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(request.RequiredFeatures));
	canonical += '|';
	canonical += request.EnableDebugInfo ? '1' : '0';
	canonical += request.EnableOptimizations ? '1' : '0';
	canonical += request.TreatWarningsAsErrors ? '1' : '0';
	canonical += request.StripDebugInfo ? '1' : '0';
	for (const std::string& define : request.Defines)
	{
		appendString(define);
	}

	for (const ShaderDescriptorBindingRemap& remap : request.DescriptorBindingRemaps)
	{
		appendString(remap.Name);
		canonical += ':';
		canonical += std::to_string(remap.Set);
		canonical += ':';
		canonical += std::to_string(remap.Binding);
		canonical += ';';
	}

	if (request.ParameterStruct)
	{
		appendString(request.ParameterStruct->AutoParametersName);
		for (const auto& value : request.ParameterStruct->Values)
		{
			appendString(value.Name);
			appendString(value.HlslType);
			canonical += std::to_string(value.OffsetInBytes) + ":" + std::to_string(value.SizeInBytes) + ";";
		}
	}

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}
