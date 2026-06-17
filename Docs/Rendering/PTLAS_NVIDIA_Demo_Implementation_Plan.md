# PTLAS NVIDIA Demo Implementation Plan

Date: 2026-06-17
Companion: `Docs/Rendering/PTLAS_TLAS_Design_Review.md`
Goal: translate the behavior and presentation of `C:\Users\pawel.stolecki\Documents\GitHub\vk_partitioned_tlas` into Sparkle's Showcase runtime as closely as practical, while staying CPU-first for correctness and using the existing profiler/debug infrastructure to prove the result.

## Target Demo Parity

Sparkle should present the same core story as NVIDIA's sample:

- Toggle PTLAS on and off live.
- Toggle classic TLAS refit on and off live when PTLAS is disabled.
- Expose `Partitions per axis`.
- Expose `Partition topology` with `2D X/Z` and `3D X/Y/Z`.
- Expose the same PTLAS partition update modes as the sample:
  - `Always update partition`
  - `Always move dynamic to global`
  - `Update partition nearby, move to global otherwise`
- Expose `Mark all dynamic in partition`.
- Expose `Mode change distance`.
- Show unique partition colors.
- Show partition activity by saturation or intensity on static geometry.
- Show TLAS/PTLAS cost differences clearly in profiler and debug overlay.
- Use a denser moving workload in Sponza, ideally many CesiumMen walking on different lanes.

## Current Gap Summary

| Area | NVIDIA sample | Sparkle today | Gap |
| --- | --- | --- | --- |
| PTLAS active toggle | Live checkbox in UI | `PTLAS Active` is now a live rendering setting | Implemented |
| TLAS refit | Live checkbox when PTLAS is off | No classic TLAS refit path exposed in renderer or RHI command list | Missing feature |
| Partitions per axis | Live UI value | Exists only as `r.RayTracing.Ptlas.PartitionsPerAxis` CVar | Missing settings UI |
| Partition topology | 2D X/Z grid plus global partition | Current planner is effectively fixed to X/Y/Z 3D | Missing topology switch |
| PTLAS update mode | Three semantic modes | No equivalent semantic mode enum; current planner uses dirty transform plus optional global partition | Missing feature |
| Mark all dynamic in partition | Supported | Not implemented | Missing feature |
| Mode change distance | Supported | Not implemented | Missing feature |
| PTLAS update behavior | Changed instances only | Full-scene `WriteInstance` pack every frame | Core behavioral gap |
| Partition visualization | Hue per partition, saturation by update activity | Per-instance hue plus dirty/moved colors | Missing parity |
| Demo workload | Dense domino motion | Sponza level includes one CesiumMan asset, showcase controller moves skeletal meshes but not as a PTLAS-specific stress demo | Too light / not targeted |

## Important Translation Decisions

### 1. Match the demo behavior before optimizing the pack path

The most important parity point is not "GPU first". It is "changed instances only".

Sparkle already has the right raw ingredients:

- Planner: `Engine/Renderer/Private/RayTracing/RayTracingTopLevelScenePlanner.cpp`
- Partition plan: `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp`
- Logical update stream: `Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp`
- Backends that can CPU-pack PTLAS op buffers:  
  `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp`  
  `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp`

The renderer is just not using those pieces selectively yet.

### 2. Expose 2D and 3D partitioning, with 3D as the more ambitious Sparkle path

NVIDIA's sample is explicitly a 2D board partitioned on X/Z:

- `vk_partitioned_tlas/src/partitioned_tlas.cpp`
- `vk_partitioned_tlas/shaders/shaderio.h`
- `vk_partitioned_tlas/shaders/raytrace.rchit.glsl`

Sparkle currently computes partition IDs from X/Y/Z and sets partition count to `PartitionsPerAxis^3`:

- `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp`

For strict sample parity, 2D X/Z is the closest translation.

For Sparkle, 3D partitioning is also a valid and potentially more interesting demo direction:

- it shows Sparkle doing something broader than the reference rather than only copying it
- it makes the partition debug view feel more engine-like and less board-specific
- it leaves room for multi-level motion in Sponza on the ground floor, stairs, balconies, and upper walkways

The cleanest plan is to expose both:

- `2D X/Z`
  - directer NVIDIA-demo parity
  - easier to read immediately
- `3D X/Y/Z`
  - more engine-forward
  - more interesting for multi-level scenes

If we do this, 3D should still be treated as an intentional product choice, not an accidental mismatch. The demo must make the extra Y dimension readable through visualization, camera placement, and actor layout.

### 3. Expose NVIDIA-style update mode, keep writer path internal

Sparkle previously exposed backend writer paths such as `CPU pack`, `GPU dirty + CPU native pack`, and `Full GPU native pack`.

Those are implementation details, not NVIDIA's semantic `Partition update mode`, and the GPU writer variants are not yet implemented enough to be trustworthy demo controls.

The UI should expose the sample-facing controls only:

- `PTLAS Active`
- `Partition update mode`

The renderer should use the known CPU-packed PTLAS operation path internally until a later GPU-native implementation is real and measurable.

## Recommended Delivery Order

1. Runtime controls and hot switching
2. Classic TLAS refit path
3. Demo-style partition planner semantics
4. Selective PTLAS CPU operation packing
5. Partition visualization parity
6. Demo scene workload
7. Optional GPU-native pack follow-up

## Stage 1: Runtime Controls And Hot Switching

### Goal

Make the sample-style controls visible in Sparkle's rendering settings and remove restart as a requirement for PTLAS on/off.

### Sparkle source touchpoints

- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp`
- `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`
- `Engine/Renderer/Private/Debug/RendererCVars.cpp`
- `Engine/RHI/Public/CVars/RHICVars.h`
- `Engine/RHI/Private/CVars/RHICVars.cpp`
- `Engine/Renderer/Private/RayTracing/RenderRayTracingScene.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingTopLevelAccelerationStructureStrategy.cpp`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/src/partitioned_tlas_ui.cpp`

### Work

- Add rendering settings and CVars for:
  - `PTLAS Active`
  - `Partitions per axis`
  - `Partition topology`
  - `Partition update mode`
  - `Mark all dynamic in partition`
  - `Mode change distance`
- Retire the visible `PTLAS update path` setting. Keep CPU operation packing as the internal implementation path.
- Recreate the top-level ray tracing strategy when PTLAS active state changes instead of requiring restart.
- Make `Partitions per axis` trigger a full PTLAS/TLAS rebuild on the next frame, not an app restart.

### Acceptance criteria

- Toggling `PTLAS Active` in settings switches between classic TLAS and PTLAS on the next frame.
- No application restart is required for `PTLAS Active`.
- `Partitions per axis` is visible in settings and takes effect after a rebuild on the next frame.
- `Partition topology` is visible in settings with `2D X/Z` and `3D X/Y/Z`, and changing it triggers a rebuild on the next frame.
- `Partition update mode`, `Mark all dynamic in partition`, and `Mode change distance` are visible in settings even before the deeper logic is wired.
- No visible `CPU pack`, `GPU dirty + CPU native pack`, or `Full GPU native pack` setting remains in Rendering Settings.

### Ready-to-use implementation prompt

```text
Implement Stage 1 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Add non-restart rendering settings and CVars for PTLAS demo controls:
- PTLAS Active
- Partitions per axis
- Partition topology
- Partition update mode
- Mark all dynamic in partition
- Mode change distance

Retire the existing visible PTLAS writer path setting. Use CPU operation packing internally until the GPU-native paths are implemented and proven.

Touch:
- Engine/Renderer/Public/Settings/EngineRenderingSettings.h
- Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp
- Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp
- Engine/Renderer/Private/Debug/RendererCVars.cpp
- Engine/RHI/Public/CVars/RHICVars.h
- Engine/RHI/Private/CVars/RHICVars.cpp
- Engine/Renderer/Private/RayTracing/RenderRayTracingScene.cpp

Important requirement:
- PTLAS Active must not require restart anymore.
- Recreate or swap the top-level ray tracing strategy/resources when the mode changes.

Do not implement the full planner logic yet if that would make the patch too large; this stage is about settings, CVars, runtime switching, and clear UX separation.
```

## Stage 2: Classic TLAS Refit Path

### Goal

Add the classic TLAS rebuild-vs-refit toggle that NVIDIA's sample exposes when PTLAS is disabled.

### Sparkle source touchpoints

- `Engine/RHI/Public/Commands/RenderCommandList.h`
- `Engine/RHI/Public/RayTracing/RhiAccelerationStructureDesc.h`
- `Engine/RHI/Private/D3D12/Commands/D3D12RenderCommandList.cpp`
- `Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingClassicTlasBuilder.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingClassicTlasStrategy.cpp`
- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/src/partitioned_tlas_ui.cpp`
- `vk_partitioned_tlas/src/partitioned_tlas.cpp`

### Work

- Add a classic TLAS build mode or explicit refit flag to the renderer/RHI interface.
- Use `UpdateScratchDataSizeInBytes` when needed instead of assuming rebuild-only scratch sizing.
- Initial frame should still perform a full build.
- Subsequent classic TLAS frames should use refit when the toggle is enabled and layout compatibility allows it.

### Acceptance criteria

- A `Refit TLAS` toggle is visible when PTLAS is disabled.
- Classic TLAS can switch between rebuild and refit live.
- GPU timing clearly changes between rebuild and refit on moving workloads.
- The mode is ignored or disabled when PTLAS is active.

### Ready-to-use implementation prompt

```text
Implement Stage 2 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Add a classic TLAS refit/update path to Sparkle's RHI and renderer.

Requirements:
- Extend RenderCommandList with a classic TLAS update/refit command or an equivalent build mode.
- Implement it for both D3D12 and Vulkan.
- Update RayTracingClassicTlasBuilder so the first build is full, later frames can use refit when enabled.
- Surface the toggle through rendering settings.
- Use the existing UpdateScratchDataSizeInBytes data where appropriate.

Touch at minimum:
- Engine/RHI/Public/Commands/RenderCommandList.h
- Engine/RHI/Private/D3D12/Commands/D3D12RenderCommandList.cpp
- Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp
- Engine/Renderer/Private/RayTracing/RayTracingClassicTlasBuilder.cpp
- Engine/Renderer/Public/Settings/EngineRenderingSettings.h
- Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp
```

## Stage 3: Demo-Style Partition Planner Semantics

### Goal

Make Sparkle's partition planner behave like the NVIDIA sample instead of just "dirty transform plus optional global partition".

### Sparkle source touchpoints

- `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.h`
- `Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingTopLevelScenePlanner.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp`
- `Engine/Renderer/Public/Diagnostics/RendererSmokeRayTracingDiagnostics.h`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/shaders/shaderio.h`
- `vk_partitioned_tlas/shaders/animation_physics.comp.glsl`
- `vk_partitioned_tlas/shaders/animation_update_instances.comp.glsl`
- `vk_partitioned_tlas/shaders/raytrace.rchit.glsl`

### Work

- Add a planner-level topology mode with `2D X/Z` and `3D X/Y/Z`.
- Add a planner-level update mode enum matching the sample.
- Add camera-distance-based switching for `Update partition nearby, move to global otherwise`.
- Add partition-level state needed for:
  - current active partition tracking
  - recent update tracking
  - `Mark all dynamic in partition`
  - move-back-from-global when a partition becomes quiet again
- Preserve stable instance identity and unique `InstanceIndex` behavior.

### Acceptance criteria

- In `2D X/Z` mode, partition count becomes `PartitionsPerAxis * PartitionsPerAxis` plus optional global partition.
- In `3D X/Y/Z` mode, partition count becomes `PartitionsPerAxis * PartitionsPerAxis * PartitionsPerAxis` plus optional global partition.
- In `2D X/Z` mode, Sponza partitions read like floor tiles.
- In `3D X/Y/Z` mode, Sponza partitions read like stable 3D cells.
- The debug view makes the chosen topology readable enough that switching between 2D and 3D feels intentional, not broken.
- `Always update partition` keeps moving instances in their local partition.
- `Always move dynamic to global` moves moving instances to global and returns them when stable.
- `Update partition nearby, move to global otherwise` uses camera distance against partition center.
- `Mark all dynamic in partition` moves all eligible instances from a touched partition into global until the partition becomes quiet.

### Ready-to-use implementation prompt

```text
Implement Stage 3 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Rework the PTLAS partition planner to match the semantics of NVIDIA's vk_partitioned_tlas sample:
- add a topology option for both 2D X/Z and 3D X/Y/Z grids
- add the three partition update modes
- add mode-change-distance camera logic
- add mark-all-dynamic-in-partition behavior
- keep global-partition return-to-local behavior when motion stabilizes

Touch:
- Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.h
- Engine/Renderer/Private/RayTracing/RayTracingPtlasPartitionPlanner.cpp
- Engine/Renderer/Private/RayTracing/RayTracingTopLevelScenePlanner.cpp
- Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp

Reference behavior from:
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\shaders\\shaderio.h
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\shaders\\animation_physics.comp.glsl
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\shaders\\animation_update_instances.comp.glsl

CPU-first is fine. Match the sample behavior first.
```

## Stage 4: Selective PTLAS CPU Operation Packing

### Goal

Stop rewriting the full scene every PTLAS frame. Use the logical update stream to submit only changed instance writes, matching the sample's behavior.

### Sparkle source touchpoints

- `Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp`
- `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp`
- `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp`
- `Docs/Rendering/PTLAS_TLAS_Design_Review.md`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/src/partitioned_tlas.cpp`
- `vk_partitioned_tlas/src/partitioned_acceleration_structures.hpp`
- `vk_partitioned_tlas/shaders/animation_update_instances.comp.glsl`

### Work

- On first build or layout change, keep a full-scene PTLAS initialization path.
- On steady-state update frames, build the PTLAS op buffer from `logicalUpdates->Records` only.
- Use a single selective `WriteInstance` operation first, because that is enough for transform updates and partition moves.
- Keep `UpdateInstance` support available for later BLAS-address-only updates, but do not block the stage on it.
- Remove `MaxOperations = 1` as a hardcoded worldview if the implementation needs more headroom, but do not invent extra ops unnecessarily.
- Make metrics truthful:
  - native op count
  - logical update count
  - validation mismatch count
  - PTLAS update GPU timing

### Acceptance criteria

- Full-scene PTLAS writes happen only on initial build, layout change, or explicit reset.
- Incremental PTLAS frames submit only changed instances.
- `logicalUpdates` and actual native write counts track each other closely.
- PTLAS update GPU time scales with moving workload instead of looking like a full rewrite every frame.
- `PackPartitionedTlasNativeOperations(...)` is no longer an empty shell if the chosen design still uses it.

### Ready-to-use implementation prompt

```text
Implement Stage 4 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Make Sparkle's PTLAS build path selective.

Current problem:
- RayTracingPartitionedTlasStrategy builds one full-scene WriteInstance pack every frame.

Target behavior:
- full-scene PTLAS write only for initial build/layout rebuild
- incremental frames use only logicalUpdates->Records
- use selective WriteInstance records with stable unique InstanceIndex values

Touch:
- Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp
- Engine/Renderer/Private/RayTracing/RayTracingPtlasLogicalUpdateStream.cpp

Use the existing backend CPU packers in:
- Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp
- Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp

Reference:
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\src\\partitioned_tlas.cpp
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\shaders\\animation_update_instances.comp.glsl
```

## Stage 5: Partition Visualization Parity

### Goal

Make Sparkle's PTLAS debug view tell the same visual story as the NVIDIA sample: unique hue per partition, stronger saturation for active partitions, and clear distinction for global-partition movement.

### Sparkle source touchpoints

- `Engine/Assets/Shaders/Debug/PTLAS/RayTracingPtlasDebugVisualization.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/VisualizeBuffers.hlsl`
- `Engine/Editor/Private/Panels/ViewportTopPanel.cpp`
- `Engine/Editor/Private/Panels/ViewportRayTracingDebugOverlay.cpp`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/shaders/raytrace.rchit.glsl`
- `vk_partitioned_tlas/README.md`

### Work

- Add per-partition activity data, not just per-instance debug flags.
- Keep `RayTracingPartitions` as pure partition hue.
- Rework `RayTracingPartitionUpdates` to show:
  - desaturated static geometry in inactive partitions
  - more saturated or brighter static geometry in active partitions
  - optional stronger highlight for partitions with more movers this frame
- Keep a distinct global-partition color treatment.
- Extend the debug overlay with:
  - current update mode
  - active partition count
  - moved-to-global count
  - classic TLAS refit state

### Acceptance criteria

- Unique partitions keep stable unique colors.
- Static geometry makes partition activity visible without needing to stare only at moving actors.
- Partition update saturation increases with actual update activity.
- Global-partition moves remain visually obvious.
- Overlay text matches the new behavior instead of the current dirty/moved-only explanation.

### Ready-to-use implementation prompt

```text
Implement Stage 5 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Rework Sparkle's PTLAS debug visualization so it matches the NVIDIA demo's communication style:
- hue identifies partition
- saturation/intensity identifies update activity
- static geometry participates in the visualization

Touch:
- Engine/Assets/Shaders/Debug/PTLAS/RayTracingPtlasDebugVisualization.hlsli
- Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl
- Engine/Editor/Private/Panels/ViewportRayTracingDebugOverlay.cpp

You will likely need a per-partition debug state source, not just the current packed per-instance flags.

Reference:
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\shaders\\raytrace.rchit.glsl
- C:\\Users\\pawel.stolecki\\Documents\\GitHub\\vk_partitioned_tlas\\README.md
```

## Stage 6: Demo Workload In Sponza

### Goal

Turn the existing Showcase content into a convincing PTLAS demo workload.

### Sparkle source touchpoints

- `Projects/Showcase/Levels/Sponza.level`
- `Projects/Showcase/Src/ShowcaseSceneController.h`
- `Projects/Showcase/Src/ShowcaseSceneController.cpp`
- `Engine/GameFramework/Private/Assets/SceneAssetManager.cpp`
- `Engine/GameFramework/Private/Level/LevelManager.cpp`

### NVIDIA reference touchpoints

- `vk_partitioned_tlas/README.md`
- `vk_partitioned_tlas/docs/demo_ptlas.png`

### Work

- Create a dedicated PTLAS demo level instead of overloading default Sponza, for example `SponzaPtlas.level`.
- Duplicate `CesiumMan/CesiumMan` scene asset loads to create many animated characters.
- Expand `ShowcaseSceneController` so each character gets a lane, phase, and offset instead of all skeletal meshes sharing one oscillation.
- Prefer routes and placements that exercise multiple Y layers in Sponza when possible so the 3D mode actually reads on screen, while still leaving enough ground-plane coverage that 2D mode looks good too.
- Keep the workload localized enough that partition activity is easy to see and profiler deltas are believable.
- Optionally add a fixed camera bookmark for before/after captures.

### Acceptance criteria

- The demo scene contains several independently moving CesiumMen in Sponza.
- Characters are spread across multiple partitions rather than stacked on one transform.
- The workload reads well in both topology modes.
- At least some of the workload demonstrates the vertical dimension of the partition grid when `3D X/Y/Z` is selected, not just X/Z spread.
- PTLAS visualization clearly shows some partitions hot and others quiet.
- The profiler and overlay change visibly when PTLAS mode changes.
- The level can be launched directly via `SPARKLE_STARTUP_LEVEL`.

### Ready-to-use implementation prompt

```text
Implement Stage 6 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Build a dedicated Sponza PTLAS demo workload using repeated CesiumMan assets and the existing ShowcaseSceneController.

Requirements:
- create a dedicated level, preferably SponzaPtlas.level
- duplicate CesiumMan scene asset loads
- modify ShowcaseSceneController so different skeletal mesh groups walk on different lanes/phases
- keep the motion readable for PTLAS partition visualization

Touch:
- Projects/Showcase/Levels/Sponza.level or a new Projects/Showcase/Levels/SponzaPtlas.level
- Projects/Showcase/Src/ShowcaseSceneController.h
- Projects/Showcase/Src/ShowcaseSceneController.cpp

Helpful source facts:
- SceneAssetManager does not deduplicate repeated SceneAssetId entries
- LevelManager supports startup-level override through SPARKLE_STARTUP_LEVEL
```

## Stage 7: Optional GPU-Native Pack Follow-Up

### Goal

Finish the advanced writer paths after the CPU-selective path is correct and demoable.

### Sparkle source touchpoints

- `Engine/Renderer/Private/RayTracing/RayTracingPtlasOperationWriterPolicy.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp`
- `Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp`
- `Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp`

### Work

- Implement `PackPartitionedTopLevelAccelerationStructureGpuOperations(...)` for supported backends.
- Wire real use of:
  - `GpuLogicalDirtyCpuNativePack`
  - `FullGpuNativePack`
- Make `FullGpuNativePackSubmitted` truthful.

### Acceptance criteria

- Requested writer path and selected writer path match actual work submitted.
- GPU pack paths no longer silently fall back without obvious reason text.
- Smoke artifacts and overlay truthfully report whether GPU native packing was actually used.

### Ready-to-use implementation prompt

```text
Implement Stage 7 of Docs/Rendering/PTLAS_NVIDIA_Demo_Implementation_Plan.md.

Finish the optional GPU PTLAS pack paths after the CPU-selective path is already working.

Requirements:
- implement backend GPU operation packing where supported
- wire the renderer to use those paths
- keep diagnostics truthful when the path is unavailable and when it is actually submitted

Touch:
- Engine/Renderer/Private/RayTracing/RayTracingPtlasOperationWriterPolicy.cpp
- Engine/Renderer/Private/RayTracing/RayTracingPartitionedTlasStrategy.cpp
- Engine/RHI/Private/D3D12/RayTracing/D3D12PartitionedTlasServices.cpp
- Engine/RHI/Private/Vulkan/RayTracing/VulkanPartitionedTlasServices.cpp
```

## Suggested Definition Of Done

The demo is ready when all of the following are true:

- `PTLAS Active` and `Refit TLAS` are live toggles.
- `Partitions per axis`, `Partition topology`, `Partition update mode`, `Mark all dynamic in partition`, and `Mode change distance` are exposed in rendering settings.
- Sparkle supports both `2D X/Z` and `3D X/Y/Z` partition topology with the same PTLAS update-mode semantics.
- PTLAS steady-state updates submit changed instances only.
- Partition colors and activity saturation are visible on static geometry.
- Sponza has a dense moving CesiumMan workload that lights up multiple partitions.
- Profiler and overlay clearly show TLAS/PTLAS differences without requiring explanation from the presenter.

## Notes

- This plan now treats partition dimensionality as a user-facing option rather than a hardcoded choice.
- If a default is needed for presentation, prefer `3D X/Y/Z` for Sparkle-specific showmanship and keep `2D X/Z` available for direct NVIDIA-demo comparison.
- If scope has to be cut, cut Stage 7 first. The CPU-selective path is the critical value.
- The existing design review already explains why the current full-scene PTLAS rewrite path is the main blocker; this document is the practical implementation companion for that review.
