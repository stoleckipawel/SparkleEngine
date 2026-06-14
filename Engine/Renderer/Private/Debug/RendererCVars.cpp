#include "PCH.h"

#include "Renderer/Public/Debug/RendererCVars.h"

ConsoleVariable<RenderViewMode> CVarRenderViewMode("r.ViewMode", RenderViewMode::Lit, "Renderer debug view mode.");
ConsoleVariable<bool> CVarRendererMeshAutoBatching(
	"r.MeshAutoBatching",
	true,
	"Build renderer-side auto batches for compatible flat mesh instances.");
ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis(
    "r.RayTracing.Ptlas.PartitionsPerAxis",
    8u,
    "Logical ray tracing partition grid resolution per axis.");
ConsoleVariable<bool> CVarRayTracingGlobalPartition(
    "r.RayTracing.Ptlas.GlobalPartition",
    true,
    "Allow dynamic ray tracing instances to move into the logical global partition.");
ConsoleVariable<ERhiPartitionedTlasOperationWriterPath> CVarRayTracingPtlasOperationWriterPath(
    "r.RayTracing.Ptlas.OperationWriterPath",
    ERhiPartitionedTlasOperationWriterPath::CpuPack,
    "PTLAS operation writer path: 1=CPU pack, 2=GPU logical dirty plus CPU native pack, 3=full GPU native pack.");
