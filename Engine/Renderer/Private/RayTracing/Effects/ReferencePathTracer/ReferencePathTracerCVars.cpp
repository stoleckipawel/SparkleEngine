#include "../../../PCH.h"
#include "RayTracing/Effects/ReferencePathTracer/ReferencePathTracerCVars.h"

ConsoleVariable<std::uint32_t> CVarReferencePathTracerSamplesPerPixel(
    "r.RayTracing.ReferencePathTracer.SamplesPerPixel",
    64u,
    "Samples per pixel evaluated by the Reference Path Tracer.");
ConsoleVariable<std::uint32_t> CVarReferencePathTracerBounceCount(
    "r.RayTracing.ReferencePathTracer.Bounces",
    8u,
    "Maximum secondary bounce count for the Reference Path Tracer.");
ConsoleVariable<float> CVarReferencePathTracerNormalBias(
    "r.RayTracing.ReferencePathTracer.NormalBias",
    0.01f,
    "World-space normal offset used by Reference Path Tracer rays.");
ConsoleVariable<float> CVarReferencePathTracerMaxDistance(
    "r.RayTracing.ReferencePathTracer.MaxDistance",
    100000.0f,
    "Maximum secondary ray distance for the Reference Path Tracer.");
