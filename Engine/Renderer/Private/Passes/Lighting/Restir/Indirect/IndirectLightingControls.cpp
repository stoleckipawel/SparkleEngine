#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "Core/Public/Console/CVar.h"
#include "Core/Public/Hash/HashUtils.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"
#include <format>

static ConsoleVariable<bool> CVarIndirectDiffuse("r.Lighting.Indirect.Diffuse", true, "Evaluate primary indirect diffuse lighting.");
static ConsoleVariable<bool> CVarIndirectSpecular("r.Lighting.Indirect.Specular", true, "Evaluate primary indirect specular lighting.");
static ConsoleVariable<bool> CVarIndirectShadows(
    "r.Lighting.Shadows.Indirect",
    true,
    "Trace secondary-hit direct-light visibility in Lit indirect transport.");

bool IsIndirectDiffuseEnabled() noexcept
{
	return CVarIndirectDiffuse.Get();
}
bool IsIndirectSpecularEnabled() noexcept
{
	return CVarIndirectSpecular.Get();
}
bool IsIndirectShadowsEnabled() noexcept
{
	return CVarIndirectShadows.Get();
}
bool IsIndirectShadowsActive() noexcept
{
	return IsIndirectShadowsEnabled() && IsIndirectLightingAdmitted();
}
bool IsIndirectLightingAdmitted() noexcept
{
	return IsIndirectDiffuseEnabled() || IsIndirectSpecularEnabled();
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
	if (!IsIndirectDiffuseEnabled())
	{
		return false;
	}
	RequireIndirectLightingProduct(input, output, "diffuse");
	return true;
}
bool IsIndirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept
{
	if (!IsIndirectSpecularEnabled())
	{
		return false;
	}
	RequireIndirectLightingProduct(input, output, "specular");
	return true;
}
RestirIndirectLightingUniformData BuildIndirectLightingUniform(
    FrameGraphTextureHandle baseColorInput,
    FrameGraphTextureHandle materialInput,
    FrameGraphTextureHandle diffuseOutput,
    FrameGraphTextureHandle specularOutput) noexcept
{
	const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
	return RestirIndirectLightingUniformData{
	    .BounceCount = settings.BounceCount,
	    .TemporalReuse = CVarRestirIndirectTemporalReuse.Get() ? 1u : 0u,
	    .SpatialReuse = CVarRestirIndirectSpatialReuse.Get() ? 1u : 0u,
	    .EvaluateDiffuse = IsIndirectDiffuseActive(baseColorInput, diffuseOutput) ? 1u : 0u,
	    .EvaluateSpecular = IsIndirectSpecularActive(materialInput, specularOutput) ? 1u : 0u,
	    .TraceSecondaryShadows = IsIndirectShadowsActive() ? 1u : 0u};
}
std::uint64_t AppendIndirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept
{
	hash = Hash::ContinueFnv1a64Value(hash, IsIndirectDiffuseEnabled());
	hash = Hash::ContinueFnv1a64Value(hash, IsIndirectSpecularEnabled());
	return Hash::ContinueFnv1a64Value(hash, IsIndirectShadowsEnabled());
}
