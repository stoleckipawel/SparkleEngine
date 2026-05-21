#include "PCH.h"

#include "Cooking/SourceIdentityHasher.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "RHI/Public/Config/RenderConfig.h"

#include <string>

std::uint64_t SourceIdentityHasher::Compute(
	const ShaderCookPackageDesc& package,
	std::span<const CookedStageBuild> compiledStages)
{
	std::string canonical;
	canonical.reserve(kSourceIdentityCanonicalReserveBytes);
	canonical += package.packageId;
	canonical += '|';
	canonical += package.bindingLayoutId;
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(package.packageKind));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(package.packageFeatures));
	canonical += '|';
	canonical += std::to_string(RenderConfig::ShaderModelMajor);
	canonical += '.';
	canonical += std::to_string(RenderConfig::ShaderModelMinor);
	canonical += '|';
	canonical += std::to_string(package.rayTracingPayloadSizeInBytes);
	canonical += '|';
	canonical += std::to_string(package.rayTracingAttributeSizeInBytes);
	canonical += '|';
	canonical += std::to_string(package.rayTracingMaxRecursionDepth);

	for (const ShaderCookRayTracingExportDesc& rtExport : package.rayTracingExports)
	{
		canonical += ";rt-export=";
		canonical += rtExport.exportLookupName;
		canonical += '|';
		canonical += std::to_string(static_cast<std::uint32_t>(rtExport.kind));
		canonical += '|';
		canonical += rtExport.exportName;
		canonical += '|';
		canonical += rtExport.entryPoint;
	}

	for (const ShaderCookRayTracingHitGroupDesc& hitGroup : package.rayTracingHitGroups)
	{
		canonical += ";rt-hit-group=";
		canonical += hitGroup.name;
		canonical += '|';
		canonical += hitGroup.closestHitExportName;
		canonical += '|';
		canonical += hitGroup.anyHitExportName;
		canonical += '|';
		canonical += hitGroup.intersectionExportName;
	}

	for (const CookedStageBuild& compiledStage : compiledStages)
	{
		canonical += ';';
		canonical += GetShaderStagePrefix(compiledStage.stage);
		canonical += '|';
		canonical += compiledStage.sourcePath;
		canonical += '|';
		canonical += compiledStage.entryPoint;
		canonical += '|';
		canonical += Formatting::FormatHexUInt64(compiledStage.bytecodeHash);
	}

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}
