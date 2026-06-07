# Ray Traced Shadows Roadmap

This plan introduces ray traced shadows in a Donut-inspired way while preserving Sparkle's existing module boundaries.

## Reference Model

- NVIDIA Donut keeps reusable render passes in `donut_render`, scene/runtime ownership in `donut_engine`, and API details behind NVRHI-style abstractions.
- Donut itself does not own ray tracing acceleration structures because AS requirements are application specific. It expects applications or samples to build AS data from scene representation and NVRHI abstractions.
- Donut-Samples includes `Ray Traced Shadows`, which rasterizes a G-buffer and renders basic ray traced directional shadows on DX12 and Vulkan.
- NVRHI's model is worth copying in spirit: resource states and barriers are explicit/trackable, binding layouts are validated against shader reflection, resource lifetime is safe, and portability is not achieved by hiding backend-specific hacks in pass code.
- AMD Cauldron/FidelityFX is useful as a second reference point: render modules own their resources and dispatches, GPU resources expose explicit state/barrier descriptions, and SDK integrations remain isolated behind renderer-facing wrappers.
- NVIDIA Real-Time Denoisers (NRD) is the correct NVIDIA integration target for denoised ray traced shadow visibility. SIGMA is the shadow-focused NRD method and should be treated as a renderer denoiser backend, not as GameFramework lighting state.
- RTXDI is a separate direct-light sampling system. It can become a later many-light strategy, but the first soft-shadow integration should not replace Sparkle's current light ownership or direct-lighting path with RTXDI.

Sparkle should follow the same spirit: keep ray tracing scene resources as renderer-owned render data, not GameFramework/importer state; keep RHI backend details behind shared RHI descriptions; keep the lighting pass composable rather than turning DirectLighting into a god pass.

## Daily Refactor Principle

Every stage in this roadmap must include a cleanup/refactor slice. The feature is not considered done if it works by increasing coupling, hiding backend behavior, or making FrameGraph harder to reason about.

Per-stage refactor checklist:

- Identify one renderer/RHI/FrameGraph responsibility that became clearer because of the stage.
- Move capability-specific details out of orchestrators and into dedicated subsystem files.
- Keep pass setup declarative: resources, bindings, barriers, and shader expectations should be inspectable without reading pass body details.
- Add or preserve diagnostics before optimizing; missing barriers, unsupported backend features, missing shader bindings, stale history, and invalid AS data must be visible.
- Remove temporary compatibility code as soon as the production path exists.
- Update this roadmap if implementation discovers a better boundary.

Reviewer gate:

- A senior reviewer should be able to answer "who owns this resource?", "who transitions it?", "who binds it?", "who destroys it?", and "what backend contract does it rely on?" from type/file names and diagnostics, not by spelunking through unrelated renderer code.

## Current Sparkle Starting Point

- RHI already exposes ray tracing capabilities through `RhiRayTracingCapabilities`.
- RHI already has bottom-level and top-level acceleration structure build commands.
- RHI already has `RhiRayTracingGeometryDesc`, `RhiRayTracingInstanceDesc`, AS prebuild info, scratch/AS/instance buffer creation, and acceleration-structure shader parameter semantics.
- D3D12 has capability reporting and AS build plumbing.
- Vulkan ray tracing capability/build parity must be audited before enabling backend parity.
- Renderer `GPUMesh` already exposes `GetRayTracingGeometry()`.
- Deferred lighting already rasterizes a G-buffer and computes direct lighting in `DirectLightingPass`, which matches the Donut-Samples shadow flow well.
- Renderer now has an explicit ray tracing capability report and ray traced shadow settings under `Engine/Renderer/Private/RayTracing`.
- Ray traced shadow CVars live in ray tracing-specific files, not the generic renderer CVar files. Current settings cover quality, denoiser, normal bias, and max distance.

## Architecture Rules

- GameFramework owns scene lights and mesh components; it must not own BLAS/TLAS resources.
- Renderer owns render-scene ray tracing resources derived from immutable scene snapshots and uploaded GPU meshes.
- RHI owns backend-neutral AS descriptions, build commands, barriers, resource views, and shader-visible bindings.
- FrameGraph owns pass resource lifetime, pass ordering, and barriers once ray traced shadows consume AS resources.
- Shaders consume a scene acceleration structure and lighting/G-buffer data; they must not know about glTF/import/cooked scene internals.
- Ray traced shadows are the engine shadow system. Do not design an engine-level shadow-map fallback, imported-vs-authored branch, or Off/Auto/Force shadow mode.
- Start with hard shadows for directional lights only. Add point/spot and soft shadows as later extensions.
- Soft shadows are a mandatory final goal, not an optional quality experiment.
- The target soft-shadow path is exactly one visibility ray per shaded pixel. Do not add a multi-sample shadow control or brute-force ray-count scaling path in the current architecture.
- Soft shadows must be represented as one-sample stochastic visibility signals plus denoiser input/output resources. Do not blur final lighting as a shortcut.
- Directional, point, and spot soft shadows should share common ray-query and denoiser plumbing while keeping light-shape sampling in light-specific shader helpers.
- NRD integration belongs behind a renderer denoiser adapter with RHI-neutral resource declarations. Do not expose NRD types through GameFramework, importer, cooker, or scene metadata.
- Prefer inline ray queries in the existing compute direct-lighting path first if both D3D12 and Vulkan can support it cleanly. Add full ray tracing pipelines/SBT only if a later feature needs ray generation/miss/hit shaders.
- FrameGraph front end and execution back end are part of this feature work. Do not route around weak FrameGraph areas with manual one-off command recording.

## FrameGraph Refactor Track

This track runs alongside every feature stage.

Front-end goals:

- Replace ambiguous pass setup with typed declarations for reads, writes, external resources, AS build/use, history resources, and imported/exported frame products.
- Make pass parameter binding validation happen at declaration time whenever possible.
- Make pass names, resource names, debug labels, and shader package/layout IDs stable and searchable.
- Keep pass orchestrators thin; feature-specific resource planning belongs in feature files such as `RayTracing`, `Denoising`, `Lighting`, or `SceneData`.

Back-end goals:

- Make resource state transitions explicit in compiled plans, including UAV and acceleration-structure barriers.
- Preserve a single source of truth for tracked resource state. Backend command lists should execute a compiled barrier plan, not infer intent from pass-specific code.
- Expose diagnostics for pass order, resource lifetime, aliasing, external resource ownership, descriptor/binding mismatch, and missing barriers.
- Keep D3D12 and Vulkan parity visible through shared plan diagnostics plus backend-specific execution validation.

Ray tracing pressure tests:

- BLAS build, TLAS build, TLAS read, shadow visibility output, NRD permanent history, NRD transient resources, denoised visibility output, and final lighting consume must all appear as first-class FrameGraph resources or explicitly documented external resources.
- If a resource cannot be represented in FrameGraph yet, the stage must add the missing FrameGraph concept instead of hiding the resource in pass code.

## Stage 0: Capability And Backend Audit

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

Goal: Establish exactly which ray tracing path Sparkle can support per backend before adding renderer feature code.

Implementation prompt:

```text
Audit Sparkle RHI ray tracing support for D3D12 and Vulkan.

Verify capability reporting, acceleration structure buffer creation, BLAS/TLAS build commands, resource barriers, acceleration-structure shader resource bindings, shader reflection/cooking metadata, and inline ray query support. Add explicit diagnostics for unsupported or incomplete backend features. Do not add renderer shadow behavior yet.
```

Acceptance criteria:

- D3D12 reports DXR and inline ray query capability accurately.
- Vulkan reports ray tracing pipeline, ray query, and acceleration structure support accurately or clearly reports unsupported.
- AS build commands validate nonzero buffers, scratch size, AS size, and GPU addresses.
- Shader compiler/reflection can detect acceleration structure parameters in compute shaders.
- Validation can run a minimal headless AS build smoke or explicitly skip unsupported backends with clear logs.
- Renderer startup logs one ray tracing capability summary and one ray traced shadow settings summary.
- Refactor gate: ray tracing capability diagnostics are centralized and no pass queries backend capabilities directly.

Implementation boundaries:

- Generic RHI validation stays in `RhiValidation.cpp`; ray tracing AS contract checks live in `RhiRayTracingValidation.cpp`.
- Vulkan device selection stays the bootstrap orchestrator; extension/feature probing lives in `VulkanRayTracingFeatureQuery`.
- Shader package loading remains responsible for package IO and common contract checks; inline ray-query metadata rules live in `ShaderRayTracingMetadataValidation`.
- Renderer startup may report capabilities and resolved ray traced shadow settings, but must not own backend feature probing or shader reflection policy.

## Stage 1: FrameGraph Front-End And Back-End Cleanup

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

Goal: Make FrameGraph strong enough to host ray traced shadows, AS resources, and NRD without manual side channels.

Implementation prompt:

```text
Refactor FrameGraph declaration, compilation, execution, and diagnostics for ray tracing readiness.

Add first-class concepts for acceleration-structure build/use resources, external persistent resources, UAV barriers, denoiser history resources, and pass binding validation. Keep the public pass declaration API small and typed. Keep backend execution as compiled plan playback rather than pass-local transition logic.
```

Suggested file structure:

- `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphAccelerationStructureRegistration.*`
- `Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompilerRayTracing.*`
- `Engine/Renderer/Private/FrameGraph/Compiler/FrameGraphCompilerExternalResources.*`
- `Engine/Renderer/Private/FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.*`
- `Engine/Renderer/Private/FrameGraph/Execution/FrameGraphBarrierPlanPlayback.*`

Acceptance criteria:

- Pass declarations can express TLAS build, TLAS read, UAV shadow visibility writes, denoiser history reads/writes, and denoiser output consumption.
- Compiled FrameGraph plans expose all transition/UAV/AS barriers in diagnostics.
- Pass parameter layouts validate declared resources against shader reflection before execution.
- D3D12 and Vulkan execution consume the same compiled plan model.
- Refactor gate: no ray tracing or denoiser feature stage may add hidden resource lifetime, barrier, or descriptor behavior outside FrameGraph unless it is explicitly documented as an external resource bridge.

Implementation boundaries:

- Public FrameGraph vocabulary owns typed texture, buffer, and acceleration-structure handles plus persistent-external import entry points.
- `PassResourceBuilder` owns setup-time declaration extraction from reflected pass layouts, including validation that acceleration structures are bound through FrameGraph handles instead of raw side-channel GPU addresses.
- `FrameGraphCompiler.cpp` remains the orchestration path; ray tracing state/barrier rules live in `FrameGraphCompilerRayTracing.*`, and external-resource boundary rules live in `FrameGraphCompilerExternalResources.*`.
- `FrameGraphResourceContractDiagnostics.*` owns pass/declaration validation and keeps contract logging out of `FrameGraphDeclaration.cpp`.
- Barrier command emission lives in `Execution/FrameGraphBarrierPlanPlayback.cpp`, leaving `FrameGraphExecution.cpp` focused on pass sequencing and diagnostics scopes.

## Stage 2: Renderer Ray Tracing Scene

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

Goal: Build renderer-owned BLAS/TLAS data from existing render mesh data.

Implementation prompt:

```text
Add a renderer ray tracing scene cache.

Build BLAS resources from uploaded GPUMesh ray tracing geometry. Build one TLAS instance per renderable mesh instance using RenderSceneData transforms and instance masks. Keep this under Renderer, with dedicated classes for BLAS cache, TLAS builder, and ray tracing scene lifetime. Do not expose AS objects through GameFramework.
```

Suggested file structure:

- `Engine/Renderer/Private/RayTracing/RenderRayTracingScene.h/.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingBlasCache.h/.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingTlasBuilder.h/.cpp`
- `Engine/Renderer/Private/RayTracing/RayTracingSceneDiagnostics.h/.cpp`

Acceptance criteria:

- Static and skeletal mesh draws produce deterministic TLAS instance counts.
- BLAS cache reuses stable `GPUMesh` geometry and rebuilds only when the mesh GPU resource changes.
- TLAS rebuild happens per frame or per scene update through Renderer-owned render data.
- Renderer logs AS instance counts, BLAS builds, TLAS builds, and backend support once per scene/load.
- No GameFramework, importer, cooker, or editor selection code references AS resources.
- Refactor gate: mesh GPU data exposes ray tracing geometry through stable renderer contracts, not ad hoc access to GPUMesh internals.

Implementation boundaries:

- `RenderRayTracingScene` is the renderer-owned scene orchestrator. It consumes `RenderSceneData` and owns no GameFramework-facing concepts.
- `RayTracingBlasCache` owns BLAS lifetime, geometry-change detection, reuse, and stale-entry eviction for `GPUMesh` inputs.
- `RayTracingTlasBuilder` owns per-frame instance gathering, TLAS scratch/result resources, instance-buffer upload, and build command recording.
- `RayTracingSceneDiagnostics` owns scene-level logging so `Renderer.cpp` stays focused on frame orchestration.
- `GPUMesh` remains the mesh upload/resource owner and exposes ray tracing geometry through its stable public contract. The ray tracing scene cache does not reach into unrelated mesh internals.

## Stage 3: FrameGraph Resource Integration

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

Goal: Make AS build/use ordering explicit instead of hidden in pass execution.

Implementation prompt:

```text
Integrate renderer ray tracing scene resources with FrameGraph.

Represent TLAS/BLAS build and consume phases with RHI-neutral resource states and barriers. Ensure acceleration structures are in build state for AS construction and ray tracing read state before direct-lighting/shadow evaluation.
```

Acceptance criteria:

- AS resources use `RayTracingAccelerationStructure` or equivalent RHI resource states.
- FrameGraph diagnostics can show ray tracing scene build before any pass that reads TLAS.
- Missing AS barriers are validation errors, not silent backend-specific behavior.
- D3D12 and Vulkan use equivalent FrameGraph ownership.
- Refactor gate: FrameGraph owns AS build/use ordering; no pass manually patches AS resource state around FrameGraph.

Implementation boundaries:

- `RenderRayTracingScene` remains the renderer-owned orchestrator for scene AS lifetime, but it now splits frame preparation from command recording so FrameGraph can own build/use ordering.
- `Frame/RayTracingScene.*` owns the authored FrameGraph pass that declares TLAS build usage and records ray tracing scene build commands through `PassRuntimeServices`.
- `FrameGraphAccelerationStructureRegistration.*` owns persistent TLAS reservation plus per-frame binding/clearing of external acceleration-structure resources; `Renderer.cpp` only binds the current frame's prepared TLAS before `FrameGraph::Setup`.
- `Frame/DirectLighting.*` owns the lighting-side dependency edge by declaring TLAS read usage when a prepared scene TLAS exists, without moving ray tracing implementation detail into `DirectLightingPass` yet.
- `RayTracingTlasBuilder` owns TLAS buffer preparation/build internals. It does not decide cross-pass ordering; FrameGraph now does that through the reserved persistent TLAS handle.

## Stage 4: Directional Ray Traced Shadow Visibility

Goal: Add hard ray traced directional shadow visibility to deferred direct lighting.

Implementation prompt:

```text
Add a ray traced directional shadow visibility path.

Use the existing G-buffer world position and normal reconstruction. For each directional light with CastShadow enabled, trace a shadow ray from the shaded point toward the light direction against the renderer TLAS. Apply a small normal-offset bias. Multiply directional light contribution by binary visibility. Keep the first implementation hard-shadow only.
```

Preferred shader shape:

- Add `RayTracedShadows.hlsli` with `TraceDirectionalShadow(...)`.
- Bind `RaytracingAccelerationStructure SceneTlas` to `DirectLightingPass` when the backend supports inline ray queries.
- Keep BRDF and light accumulation in `DirectLightingCommon.hlsli`.
- Keep ray tracing helpers out of general G-buffer utilities.

Acceptance criteria:

- Directional lights with `CastShadow = true` cast ray traced hard shadows.
- Directional lights with `CastShadow = false` do not cast shadows.
- If ray tracing is unavailable, renderer startup emits a clear error because no alternate engine shadow path is expected.
- No shadow-map compatibility layer is introduced in this stage.
- D3D12 runtime smoke shows visible/deterministic shadow contribution.
- Vulkan either validates the same path or emits a clear capability skip if Stage 0 proves missing support.
- Refactor gate: DirectLighting remains lighting orchestration; ray query helpers and shadow signal code live in dedicated shadow shader/source files.

## Stage 5: Shadow Settings

Goal: Keep ray traced shadow settings centralized without spreading feature knobs through unrelated systems.

Implementation prompt:

```text
Add ray tracing subsystem shadow settings.

Expose ray traced shadow quality, denoiser, shadow ray bias, max ray distance, and diagnostic controls from files owned by the ray tracing subsystem. Keep light ownership in GameFramework and shadow evaluation settings in Renderer.
```

Acceptance criteria:

- Ray traced shadow CVars are declared and defined in ray tracing-specific files.
- Settings are read by Renderer ray tracing setup, not by GameFramework light classes.
- Individual passes must consume resolved ray traced shadow settings or pass data, not read CVars directly.
- The resolved settings always report `raysPerPixel=1`; this is a core design target, not a tunable quality slider.
- Missing backend capability is an error diagnostic for the shadow system, not a request to silently use another shadow technique.
- Refactor gate: generic renderer CVar files do not accumulate feature-specific settings.

## Stage 6: Local Light Shadow Extension

Goal: Extend ray traced shadows beyond directional lights once the directional path is stable.

Implementation prompt:

```text
Add point and spot ray traced hard shadows.

Trace finite-distance rays from shaded points toward point and spot light positions. Respect light range and spot cone visibility. Share the shadow tracing helper but keep light-type math in light-specific functions.
```

Acceptance criteria:

- Point and spot lights can cast ray traced hard shadows.
- Range and cone angles affect both illumination and shadow ray max distance.
- Existing 512-light support remains bounded and diagnosable.
- Shader code remains organized by light type and common ray query helper.
- Refactor gate: local-light shadow data extends renderer light records without adding directional-light assumptions to shared lighting code.

## Stage 7: Stochastic Soft Shadow Visibility

Goal: Generate physically meaningful one-ray-per-pixel noisy soft-shadow visibility for directional, point, and spot lights before denoising.

Implementation prompt:

```text
Add stochastic ray traced soft-shadow visibility.

Represent light size explicitly in renderer-facing light data: angular diameter for directional lights, radius for point lights, and radius/cone description for spot lights. Sample the appropriate light shape once per shaded pixel and trace one visibility ray against TLAS. Output a shadow visibility signal and hit-distance/confidence metadata suitable for denoising. Keep direct lighting evaluation separate from the raw visibility generation.
```

Acceptance criteria:

- Directional lights support angular soft shadows without treating the sun as a point light.
- Point and spot lights support finite-area soft shadows with range-aware ray distances.
- Every shaded pixel traces at most one shadow visibility ray in the current soft-shadow path.
- Light shape data is renderer-facing lighting data, not imported-vs-authored special behavior.
- The first stochastic output is noisy but physically interpretable and deterministic under fixed seeds.
- Hard-shadow quality still bypasses stochastic sampling and denoiser resources.

Suggested shader structure:

- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowSignals.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowDenoiserInputs.hlsli`
- Refactor gate: stochastic visibility generation is a shadow-signal stage; final lighting should consume a visibility result and not own sampling details.

## Stage 8: NVIDIA NRD SIGMA Integration

Goal: Integrate NVIDIA NRD SIGMA as the production denoiser path for soft ray traced shadows.

Implementation prompt:

```text
Add an NRD integration layer for ray traced shadow denoising.

Introduce a Renderer-owned NRD adapter that translates Sparkle FrameGraph resources into NRD dispatches. Feed SIGMA-compatible shadow visibility, hit distance or equivalent confidence data, view depth, normals, motion vectors when available, and camera jitter/history metadata. Keep NRD resource lifetime and dispatch scheduling in Renderer/FrameGraph. Keep NRD-specific headers and library calls out of RHI, GameFramework, import, cook, and shader authoring surfaces except for dedicated denoiser shader/resource wrappers.
```

Suggested file structure:

- `Engine/Renderer/Private/Denoising/Nrd/NrdDenoiserContext.h/.cpp`
- `Engine/Renderer/Private/Denoising/Nrd/NrdSigmaShadowPass.h/.cpp`
- `Engine/Renderer/Private/Denoising/ShadowDenoiserPolicy.h/.cpp`
- `Engine/Renderer/Private/Denoising/ShadowDenoiserResources.h/.cpp`

Acceptance criteria:

- NRD is an optional renderer dependency guarded by CMake feature flags and runtime capability diagnostics.
- D3D12 and Vulkan use equivalent FrameGraph resource declarations and denoiser dispatch scheduling.
- NRD SIGMA can denoise directional and local-light soft-shadow visibility.
- When NRD is unavailable, `SoftAreaLights + NrdSigma` emits a clear renderer diagnostic. The implementation may temporarily use hard ray traced shadows while NRD is absent, but it must not introduce a second engine shadow path.
- Direct lighting consumes denoised visibility, not NRD-owned resources directly.
- History reset works on resize, camera cut, level switch, and denoiser setting changes.
- Refactor gate: NRD integration adds a denoiser adapter and FrameGraph resource declarations; it must not add NVIDIA-specific concepts to RHI or generic pass code.

## Stage 9: Temporal Inputs And Motion Vectors

Goal: Provide the temporal data needed for stable soft-shadow denoising under camera and object motion.

Implementation prompt:

```text
Add renderer temporal inputs required by NRD.

Track previous view/projection, camera jitter, per-pixel motion vectors, history validity, and responsive resets. Add object motion for moving/skinned meshes once animation ray tracing geometry handling exists. Keep temporal resource ownership in FrameGraph and renderer frame state.
```

Acceptance criteria:

- NRD history resets deterministically on level switch and resize.
- Static camera soft shadows converge without visible instability.
- Moving camera soft shadows remain stable using camera motion vectors.
- Animated or deforming shadow casters either provide object motion or emit a clear reduced-quality diagnostic.
- Refactor gate: temporal frame state has a single owner and reset path shared by denoisers/upscalers/future temporal effects.

## Stage 10: Deforming Mesh And Animation Handling

Goal: Decide how animated skeletal/morph meshes participate in acceleration structures without muddying static mesh paths.

Implementation prompt:

```text
Add explicit ray tracing geometry update handling for skeletal and morphing meshes.

Keep static BLAS stable. For skeletal/morphing meshes, choose either CPU-deformed mesh upload reuse, refit/rebuild BLAS per geometry change, or exclude animated meshes from shadow casters with diagnostics until a scalable path exists.
```

Acceptance criteria:

- Static meshes do not pay skeletal/morph AS rebuild cost.
- Skeletal/morph meshes have explicit diagnostics for included, rebuilt, refit, or excluded state.
- Renderer does not infer AS update behavior from importer data.
- Refactor gate: static and deformable ray tracing geometry paths are explicit and do not pollute each other's mesh upload or AS update code.

## Stage 11: Quality, Performance, And Many-Light Handling

Goal: Improve quality and scalability after hard and denoised soft shadows are correct.

Implementation prompt:

```text
Add quality/performance handling for denoised soft shadows.

Tune blue-noise/sample sequence quality, checkerboard/half-res options if needed, light caps, tile classification, and local-light shadow budgeting while preserving the one-ray-per-pixel target. Consider RTXDI only as a later direct-light sampling system if the number of shadowed local lights outgrows simple bounded loops.
```

Acceptance criteria:

- Hard-shadow path remains available and deterministic.
- Soft-shadow and denoiser settings are renderer-owned.
- Denoising resources are FrameGraph-owned.
- Performance and quality tradeoffs are measured before claiming a win.
- Local-light shadow budgeting remains explicit when many of the 512 lights cast shadows.
- Refactor gate: any budget/culling path is represented as renderer scene data or FrameGraph inputs, not hidden inside a shader permutation or pass-local loop.

## Validation Matrix

Minimum commands:

```powershell
cmake --build build-codex-ninja --target ShowcaseRuntime ShowcaseEditor ShaderCompiler -- -j 1
```

Required smokes:

- D3D12 runtime smoke with a simple occluder/receiver scene and one shadow-casting directional light.
- Vulkan runtime smoke for the same scene or a clear capability skip with diagnostic proof.
- Existing scenes with shadow casting disabled at the light level.
- `CameraCubeImportedCamera` or another imported-light scene to prove glTF/imported lights use the same light and shadow path.
- `SkeletalMorphTriangle` once Stage 10 decides animated/deformable AS handling.
- A directional soft-shadow scene with large penumbra and camera motion.
- A local-light soft-shadow scene with point and spot lights at different radii/ranges.
- NRD unavailable and enabled diagnostics.

Log evidence to require:

- Backend ray tracing capability summary.
- BLAS build/reuse count.
- TLAS instance count.
- Shadow quality selected: Hard or SoftAreaLights.
- Denoiser selected: Off or NrdSigma, plus whether the NRD backend initialized.
- DirectLighting or shadow visibility pass binding includes TLAS when shadow evaluation runs.
- Denoiser pass resources include visibility, depth, normal, motion/history, and output visibility when NRD is enabled.
- FrameGraph diagnostics list AS build/use barriers, UAV barriers, denoiser history resources, and final lighting dependencies.

## Non-Goals For First Changelist

- No path tracing.
- No ray traced reflections.
- No soft shadows in the first hard-shadow changelist. Soft shadows are a dedicated follow-up track with NRD integration.
- No RTXDI/ReSTIR.
- No GameFramework-owned AS state.
- No imported-vs-authored shadow behavior branches.
- No backend-specific renderer shortcuts hidden behind `if D3D12` in pass code.
- No manual command recording side channels to bypass weak FrameGraph areas.

## Open Design Questions

- Should first implementation use inline ray query inside `DirectLighting.hlsl`, or a separate shadow mask compute pass that DirectLighting samples?
- Should TLAS be rebuilt every frame initially for simplicity, then optimized, or should static/dynamic partitioning be introduced immediately?
- How much Vulkan ray tracing support is already implemented versus only described by RHI types?
- Should alpha-tested geometry be opaque in first pass, or should any-hit/alpha testing be deferred until material opacity data is bound for ray tracing?
- Which NRD package version should be vendored, and should it live under a CMake option such as `SPARKLE_ENABLE_NRD`?
- Should point/spot soft shadows be capped per frame independently from lighting count to avoid tracing and denoising too many of the 512 punctual lights?
- Do we need shadow visibility atlases/tiled denoising for many local lights, or is one screen-space aggregate visibility signal enough for the first NRD SIGMA integration?
