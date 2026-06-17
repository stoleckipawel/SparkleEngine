#include "PCH.h"

#include "Renderer/Public/Debug/RendererCVars.h"

ConsoleVariable<RenderViewMode> CVarRenderViewMode("r.ViewMode", RenderViewMode::Lit, "Renderer debug view mode.");
ConsoleVariable<bool> CVarRendererMeshAutoBatching(
	"r.MeshAutoBatching",
	true,
	"Build renderer-side auto batches for compatible flat mesh instances.");
ConsoleVariable<bool> CVarRayTracingClassicTlasRefit(
    "r.RayTracing.Tlas.Refit",
    true,
    "Use classic TLAS refit/update after the initial full build when PTLAS is disabled.");
ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis(
    "r.RayTracing.Ptlas.PartitionsPerAxis",
    8u,
    "Logical ray tracing partition grid resolution per axis.");
ConsoleVariable<RayTracingPtlasPartitionTopology> CVarRayTracingPtlasPartitionTopology(
    "r.RayTracing.Ptlas.PartitionTopology",
    RayTracingPtlasPartitionTopology::XYZ3D,
    "PTLAS partition topology: 0=2D X/Z, 1=3D X/Y/Z.");
ConsoleVariable<RayTracingPtlasPartitionUpdateMode> CVarRayTracingPtlasPartitionUpdateMode(
    "r.RayTracing.Ptlas.PartitionUpdateMode",
    RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition,
    "PTLAS partition update mode: 0=always update partition, 1=always move dynamic to global, 2=update nearby and move distant dynamic to global.");
ConsoleVariable<bool> CVarRayTracingPtlasMarkAllDynamicInPartition(
    "r.RayTracing.Ptlas.MarkAllDynamicInPartition",
    false,
    "Marks all dynamic instances in a touched partition as dynamic, matching the NVIDIA PTLAS demo control.");
ConsoleVariable<float> CVarRayTracingPtlasModeChangeDistance(
    "r.RayTracing.Ptlas.ModeChangeDistance",
    100.0f,
    "Distance threshold for switching dynamic instances between nearby partition updates and the global partition.");
