#include "PCH.h"

#include "Cooking/SourceIdentityHasher.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Hash/HashUtils.h"
#include "RHI/Public/Config/RenderConfig.h"

#include <format>
#include <string>

std::uint64_t SourceIdentityHasher::Compute(
	const ShaderCookPackageDesc& package,
	std::span<const CookedStageBuild> compiledStages)
{
	std::string canonical;
	canonical.reserve(kSourceIdentityCanonicalReserveBytes);
	canonical += package.packageId;
	canonical += '|';
	canonical += package.variantId;
	canonical += '|';
	canonical += package.bindingLayoutId;
	canonical += '|';
	canonical += std::to_string(RenderConfig::ShaderModelMajor);
	canonical += '.';
	canonical += std::to_string(RenderConfig::ShaderModelMinor);

	for (const CookedStageBuild& compiledStage : compiledStages)
	{
		canonical += ';';
		canonical += GetShaderStagePrefix(compiledStage.stage);
		canonical += '|';
		canonical += compiledStage.sourcePath;
		canonical += '|';
		canonical += compiledStage.entryPoint;
		canonical += '|';
		canonical += std::format("{:016X}", compiledStage.bytecodeHash);
	}

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}
