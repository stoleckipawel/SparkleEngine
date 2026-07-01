#include "../../../PCH.h"
#include "RayTracing/Effects/ReferencePathTracing/ReferencePathTracingCVars.h"

ConsoleVariable<bool> CVarReferencePathTracingEnabled(
    "r.RayTracing.ReferencePathTracing.Enabled",
    false,
    "Enable the high-sample reference path tracing pass.");
ConsoleVariable<std::uint32_t> CVarReferencePathTracingSamplesPerPixel(
    "r.RayTracing.ReferencePathTracing.SamplesPerPixel",
    64u,
    "Samples per pixel accumulated by the reference path tracing pass.");
ConsoleVariable<std::uint32_t> CVarReferencePathTracingBounceCount(
    "r.RayTracing.ReferencePathTracing.Bounces",
    8u,
    "Maximum path bounce count for reference path tracing.");
ConsoleVariable<float> CVarReferencePathTracingNormalBias(
    "r.RayTracing.ReferencePathTracing.NormalBias",
    0.01f,
    "World-space normal offset used by reference path tracing rays.");
ConsoleVariable<float> CVarReferencePathTracingMaxDistance(
    "r.RayTracing.ReferencePathTracing.MaxDistance",
    100000.0f,
    "Maximum ray distance for reference path tracing rays.");
