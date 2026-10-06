#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Hash/HashUtils.h"
#include <format>

ConsoleVariable<bool> CVarIndirectDiffuse("r.Lighting.Indirect.Diffuse", true, "Evaluate primary indirect diffuse lighting.");
ConsoleVariable<bool> CVarIndirectSpecular("r.Lighting.Indirect.Specular", true, "Evaluate primary indirect specular lighting.");
ConsoleVariable<bool> CVarIndirectShadows(
    "r.Lighting.Shadows.Indirect",
    true,
    "Trace secondary-hit direct-light visibility in Lit indirect transport.");

bool IsIndirectShadowsActive() noexcept
{
	return CVarIndirectShadows.Get() && IsIndirectLightingAdmitted();
}

bool IsIndirectLightingAdmitted() noexcept
{
	return CVarIndirectDiffuse.Get() || CVarIndirectSpecular.Get();
}

static void RequireIndirectLightingProduct(FrameGraphTextureHandle input, FrameGraphTextureHandle output, const char* lobe) noexcept
{
	if (!input.IsValid() || !output.IsValid())
	{
		SPARKLE_DEFINE_LOG_CATEGORY_STATIC(LogIndirectLighting, "Renderer.IndirectLighting");
		Diagnostics::Fatal(
		    LogIndirectLighting,
		    __FILE__,
		    __LINE__,
		    std::format("Enabled indirect {} requires its GBuffer input and lighting output.", lobe));
	}
}

bool IsIndirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!CVarIndirectDiffuse.Get())
	{
		return false;
	}
	RequireIndirectLightingProduct(input, output, "diffuse");
	return true;
}

bool IsIndirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!CVarIndirectSpecular.Get())
	{
		return false;
	}
	RequireIndirectLightingProduct(input, output, "specular");
	return true;
}

std::uint64_t AppendIndirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept
{
	hash = Hash::ContinueFnv1a64Value(hash, CVarIndirectDiffuse.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarIndirectSpecular.Get());
	return Hash::ContinueFnv1a64Value(hash, CVarIndirectShadows.Get());
}
