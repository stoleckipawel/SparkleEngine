#include "../PCH.h"
#include "RayTracing/RayTracedShadowCVars.h"

ConsoleVariable<RayTracedShadowQualityMode> CVarRayTracedShadowQualityMode(
	"r.RayTracedShadows.Quality",
	RayTracedShadowQualityMode::SoftAreaLights,
	"Ray traced shadow quality: 0=Hard, 1=SoftAreaLights.");
ConsoleVariable<RayTracedShadowDenoiserMode> CVarRayTracedShadowDenoiserMode(
	"r.RayTracedShadows.Denoiser",
	RayTracedShadowDenoiserMode::NrdSigma,
	"Ray traced shadow denoiser: 0=Off, 1=NRD SIGMA.");
ConsoleVariable<float> CVarRayTracedShadowNormalBias(
	"r.RayTracedShadows.NormalBias",
	0.01f,
	"World-space normal offset used by ray traced shadow rays.");
ConsoleVariable<float> CVarRayTracedShadowMaxDistance(
	"r.RayTracedShadows.MaxDistance",
	100000.0f,
	"Maximum ray distance for directional ray traced shadows.");
