# Measurement Cleanup Validation

Purpose: verify Sparkle's profiler-first measurement contract stays clean as renderer features evolve.

## Expected State

- Frame graph owns normal render-pass GPU scopes.
- Feature code uses `SPARKLE_GPU_SCOPE` only for meaningful nested GPU work that frame graph cannot infer.
- Feature code uses `SPARKLE_CPU_SCOPE` for CPU work that subsystem automation cannot infer.
- Ray tracing and feature diagnostics do not own CPU/GPU millisecond fields.
- Smoke artifacts export generic renderer GPU timings by scope label instead of per-effect timing fields.
- Backend-specific marker calls remain behind RHI/diagnostics boundaries.

## Validation Queries

Run from the repository root:

```powershell
rg -n "\.Timings|\.CpuMilliseconds|\.GpuMilliseconds|InstancePreparationCpuMilliseconds|CpuPackMilliseconds|GpuDirtyDetectionMilliseconds|GpuNativePackMilliseconds|PtlasUpdateGpuMilliseconds|RayTracingPassGpuMilliseconds|IndirectSpecularGpuMilliseconds|RendererSmokeRayTracingFrameTimingDiagnostics|RayTracingFrameTimingMetrics|BeginResolvedGpuTimingFrame|PublishResolvedGpuTiming" Engine Tools -g "*.h" -g "*.cpp"
```

Expected: no matches.

```powershell
rg -n "BeginGpuEvent\(|BeginGpuTimer\(|BeginTimer\(|SPARKLE_GPU_PASS_SCOPE" Engine/Renderer/Private/Passes Engine/Renderer/Private/FramePipeline Engine/Renderer/Private/FrameGraph Engine/Renderer/Private/RayTracing -g "*.h" -g "*.cpp"
```

Expected: no matches in feature/pass/frame assembly code.

```powershell
rg -n "BeginGpuEvent\(|BeginGpuTimer\(|BeginTimer\(" Engine/Renderer Engine/Editor Engine/Application -g "*.h" -g "*.cpp"
```

Expected intentional exceptions only:

- `FrameExecutionDiagnostics`: internal implementation for `ScopedGpuScope`.
- `PassExecutionDiagnostics`: compatibility wrappers and frame-graph-owned pass scope internals until the raw APIs are deleted.

## Current Intentional Exceptions

- `RendererSmokeFrameTimingDiagnostics::FinalFrameGpuMilliseconds` remains as the generic smoke-level final frame summary.
- Tool cooking elapsed-millisecond summaries remain because they are CLI/report artifacts, not renderer profiler duplication.
- `FrameExecutionDiagnostics::BeginGpuEvent` and `BeginTimer` remain internal plumbing for `ScopedGpuScope`; feature code should not call them directly.
- `PassExecutionDiagnostics::BeginGpuEvent` and `BeginTimer` remain temporary compatibility wrappers. Prefer deleting them after all callers use `BeginGpuScope`/`SPARKLE_GPU_SCOPE`.

