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

## Contract-first design (applies to all renderer feature stages)

To avoid hidden coupling, each pass should define a contract that is explicit in code:

- **Inputs**: resources it consumes (for example G-buffer, depth, normals, TLAS, motion vectors).
- **Outputs**: resources it produces or writes (for example visibility buffers, denoised visibility, lighting targets).
- **Assumptions**: required states or flags (for example TLAS build/read state, history validity, max light counts).
- **Ownership rules**: which subsystem creates and transitions each resource.

This roadmap uses contracts so feature code can be added by adding/expanding the relevant contract and then implementing against the declared interfaces.
A contract is not extra complexity by itself; it is a structure that makes ownership, ordering, and fallback behavior readable.

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

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

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

Implementation boundaries:

- `DirectLightingPass` remains the pass-level orchestrator. It binds the scene TLAS and resolved shadow settings but does not own ray-query math.
- `RenderRayTracingPassServices` groups the execute-time ray tracing scene pointer and resolved shadow settings so generic pass runtime services do not accumulate per-feature fields.
- `Passes/Deferred/RayTracedShadows.hlsli` owns directional shadow-ray setup, normal-bias application, and inline ray-query visibility evaluation.
- `Passes/Deferred/DirectLightingCommon.hlsli` remains the BRDF and light-accumulation home. It only consumes a scalar visibility term for directional lighting.
- `RayTracedShadowSettings.*` and `RayTracedShadowUniformData` own subsystem shadow settings and the pass-facing uniform payload. GameFramework light records still only provide `CastShadow`.
- Renderer pipeline capability validation treats `DirectLightingPass` as an inline-ray-query pass through explicit required shader-package features instead of relying on hidden backend assumptions.

## Stage 5: Shadow Settings

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

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

Implementation boundaries:

- `RayTracedShadowCVars.*` owns ray tracing shadow feature CVars, including diagnostics. No generic renderer CVar file should gain shadow-specific state.
- `RayTracedShadowSettings.*` owns resolved subsystem settings and startup logging, including the fixed `raysPerPixel=1` contract.
- `RenderRayTracingPassServices` is the execute-time bridge from renderer-owned ray tracing subsystem state into passes.
- `RayTracedShadowPassData.*` owns translation from resolved subsystem settings into per-pass uniform payloads so lighting passes do not interpret CVars or shadow policy directly.

## Stage 6: Local Light Shadow Extension

Status: Source implemented. Build validation is currently blocked by the local MSVC environment failing to locate standard library headers before compiling the changed code.

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

Implementation boundaries:

- Renderer-facing `PointLight` and `SpotLight` records carry `castShadow` as part of normal lighting data; there is no imported-vs-authored shadow branch.
- `RenderLightingBuilder` and `ViewLightingBuilder` propagate local-light shadow capability into renderer scene data and per-view lighting buffers.
- `RayTracedShadows.hlsli` owns the common shadow-ray helper plus local-light-specific trace entry points. `DirectLightingCommon.hlsli` only consumes scalar visibility terms during BRDF accumulation.
- Point and spot shadow math stays in light-specific functions and uses local-light range/cone information without adding directional-only assumptions to shared lighting helpers.

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

## Stage 8: Temporal + AA Readiness Before Denoiser

Goal: Provide a stable temporal and anti-aliasing foundation before moving into NRD denoiser stages.

The denoiser chain below depends on motion vectors, history buffers, and temporal contract ownership.

### Stage 8.1: Temporal Inputs And Motion Vectors

Status: In progress.

Goal: Provide temporal data required by shadow denoising and temporal upscaling.

Implementation prompt:

```text
Track previous view/projection, camera jitter, per-pixel motion vectors, history validity, and responsive resets.
Keep temporal ownership in renderer frame state and FrameGraph resource lifetime.
```

Acceptance criteria:

- Motion vectors are available for temporal passes and shadow-consumption flows.
- Temporal history resets deterministically on resize, level switch, and camera cuts.
- Missing inputs are logged and do not silently enable incomplete temporal paths.
- Refactor gate: temporal state stays in renderer frame systems and shared contracts, not in lighting or denoiser passes.

Current implementation status (Stage 8.1):

- Added per-frame temporal state tracking for previous view/projection matrices, history validity, and history reset requests.
- Motion vectors are now emitted from G-Buffer as `MotionVector`.
- Temporal history resets are driven by renderer- and level-lifecycle events (resize, extent change, level change/unload) and deterministic camera-cut heuristics.
- Camera-cut reset diagnostics now log position delta, direction dot, and FOV delta.
- `RenderViewData` now exposes a provider-neutral temporal frame summary with current/previous jitter and history validity, keeping future DLSS/NRD consumers away from pass-local temporal internals.

### Stage 8.2: Production DLSS Integration

Goal: integrate NVIDIA DLSS Super Resolution as the production temporal upscaler and anti-aliasing provider while using the integration to mature Sparkle's renderer, RHI, FrameGraph, pass orchestration, and runtime feature architecture.

Implementation prompt:

```text
Replace the planned in-house temporal anti-aliasing baseline with a production DLSS integration using NVIDIA Streamline or NGX.
Treat DLSS as a first-class renderer feature provider, not as an ordinary authored shader pass.
Refactor pass orchestration, FrameGraph contracts, and RHI capability reporting so vendor upscalers can be scheduled, validated, and disabled cleanly.
```

Reference posture:

- Primary references:
  - NVIDIA Streamline repository and programming guide: `https://github.com/NVIDIA-RTX/Streamline`
  - NVIDIA Streamline DLSS programming guide: `https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS.md`
  - NVIDIA Streamline sample: `https://github.com/NVIDIA-RTX/Streamline_Sample`
  - NVIDIA Vulkan Streamline sample: `https://github.com/nvpro-samples/vk_streamline`
  - NVIDIA DLSS programming guide: `https://github.com/NVIDIA/DLSS/blob/main/doc/DLSS_Programming_Guide_Release.pdf`
- Prefer NVIDIA Streamline as the integration surface unless a backend-specific constraint requires direct NGX.
- Treat NVIDIA references as production architecture pressure, not as names to copy into Sparkle's general renderer. Inherit the practices: explicit resource contracts, provider-owned SDK lifetime, narrow native-handle bridges, capability diagnostics, deterministic fallback, and presentation through a renderer-owned final product.
- Follow NVIDIA's DLSS integration checklist inside the DLSS provider: tagged color, depth, motion-vector, exposure/HDR, jitter, reset, frame-index, and resource-state inputs must be valid at evaluation time.
- DLSS replaces the primary scene upscale/resolve path only. It must not be used for secondary buffers such as shadow, reflection, or denoiser intermediate resources.
- General renderer, RHI, FrameGraph, temporal, lighting, denoiser, and presentation code must stay provider-neutral. DLSS, Streamline, NGX, and NVIDIA SDK names belong only inside the NVIDIA provider module and documentation.

Implementation phases:

#### Phase 8.2.1: Reference Study And Integration Decision

Work:

- Review NVIDIA Streamline, Streamline Sample, `vk_streamline`, and the DLSS programming guide before code changes.
- Write a short implementation record in `docs/` that extracts production practices from NVIDIA references and turns them into Sparkle architecture requirements: explicit resource contracts, command-list hooks, viewport/render extents, jitter, motion-vector convention, reset state, final output product, fallback behavior, and ownership boundaries.
- Decide Streamline-first versus direct NGX and record why. Default decision should be Streamline-first for production DLSS unless a backend constraint blocks it.

Acceptance criteria:

- The implementation record identifies required SDK calls, required resources, required per-frame constants, unsupported backends, SDK binary/runtime dependencies, and the fallback path.
- The record explicitly states which Sparkle layer owns SDK initialization, feature lifetime, quality mode, command-list evaluation, and resource tagging.
- No renderer code is changed in this phase unless needed to add documentation anchors or TODO records.

Validation:

- Documentation review only. No build required.
- Cross-check against NVIDIA references listed above and record the versions/commits used.

Current implementation status (Phase 8.2.1):

- Added `docs/plans/dlss-integration-implementation-record.md` as the reference-backed implementation record.
- Recorded the integration decision: Streamline-first for production DLSS, with direct NGX kept only as a fallback investigation path if Streamline blocks a required backend or packaging model.
- Recorded Sparkle ownership boundaries for SDK initialization, feature lifetime, quality mode, command-list evaluation, FrameGraph resource contracts, RHI native handles, fallback behavior, and presentation output.
- Added separation rules so DLSS-specific names and SDK details stay inside the NVIDIA provider module while general renderer code receives only reusable architecture concepts.

#### Phase 8.2.2: RHI Capability And External Feature Surface

Work:

- Add RHI capability reporting for external upscalers: backend API support, native device/queue/command-list handle availability, adapter identity, driver/runtime availability, and supported SDK feature flags.
- Separate "backend can expose native handles" from "DLSS provider is available." DLSS availability depends on SDK/runtime/device checks and must not be hard-coded to the RHI backend alone.
- Add diagnostics that explain why DLSS is unavailable: backend unsupported, SDK missing, adapter unsupported, feature query failed, invalid resource contract, or user disabled.

Acceptance criteria:

- Renderer startup can report a structured DLSS capability summary without creating any DLSS feature instance.
- RHI capability structs remain vendor-neutral enough to support future external providers.
- Native handle access stays behind RHI interfaces; renderer systems do not cast backend objects directly.
- No NVIDIA-specific SDK fields, enums, or handles are added to generic RHI capability structs.

Validation:

- Header/API review and diagnostic text review. No full build required until final integration.
- Unit-style compile validation can be deferred if local toolchain is unavailable, but the expected compile target must be listed in the phase record.

#### Phase 8.2.3: Renderer Upscaler Provider Boundary

Work:

- Introduce a renderer-owned `Upscaler` or `SuperResolution` subsystem with provider selection: `None`, deterministic passthrough, and NVIDIA DLSS Super Resolution.
- Define provider interfaces for initialization, capability query, per-frame setup, evaluation, resize/reset, shutdown, and diagnostic capture.
- Keep SDK lifetime, SDK feature handles, quality mode, dynamic-resolution state, and reset handling inside the provider subsystem.
- Do not expose NVIDIA SDK headers through general renderer, RHI, FrameGraph, lighting, or presentation headers.
- If the provider needs renderer data that does not exist yet, add provider-neutral contracts first instead of threading DLSS-specific fields through `FrameContext` or frame target structs.

Acceptance criteria:

- The renderer can select a provider without changing lighting, shadow denoising, G-buffer, or presentation code.
- DLSS settings are not stored in generic `FrameContext` unless they are provider-agnostic render settings.
- Fallback output remains deterministic and produces the same scene color product as the pre-DLSS renderer.
- General frame orchestration schedules the provider through an upscaler interface and does not calculate DLSS constants, tags, or reset rules inline.

Validation:

- Static architecture review: includes, ownership, and dependency direction.
- No build required until final integration.

#### Phase 8.2.4: Temporal And Resource Input Contract

Work:

- Define a typed `UpscalerInputContract` containing HUD-less scene color, depth, motion vectors, exposure/HDR metadata, jitter, render extent, display extent, frame index, reset state, and camera-cut state.
- Validate motion-vector convention against NVIDIA expectations: units, sign, jitter inclusion/exclusion, resolution scaling, and depth range.
- Make temporal jitter ownership explicit: temporal state generates jitter; DLSS consumes it; provider code must not duplicate jitter generation.
- Keep temporal derivation out of general frame assembly: shared temporal summaries should be built by temporal/frame helpers and consumed through provider-neutral contracts.

Acceptance criteria:

- Missing required DLSS inputs are logged and force deterministic fallback.
- Motion-vector convention is documented in code and docs with a validation scene plan.
- Resize, quality-mode change, scene cut, invalid history, and device change all flow into a provider reset/recreate path.

Validation:

- Contract review and diagnostic-path review. No full build required until final integration.

#### Phase 8.2.5: FrameGraph External Provider Evaluation

Work:

- Add explicit FrameGraph concepts for external provider evaluation rather than modeling DLSS as a normal shader pass.
- Represent provider reads and writes with typed declarations: color input, depth input, motion-vector input, exposure input, output color, required states, external SDK call, and barrier assumptions.
- Ensure FrameGraph diagnostics can print provider inputs, output, scheduling position, resource states, and fallback reason.
- Make presentation consume a selected `FinalSceneColor` product, not a hard-coded scene color texture.

Acceptance criteria:

- DLSS scheduling is visible in FrameGraph diagnostics even though the SDK performs the work externally.
- Provider evaluation cannot accidentally read lighting internals, denoiser intermediates, or editor/HUD overlays.
- Non-DLSS frame graphs remain stable and do not allocate provider-only resources.

Validation:

- FrameGraph contract/diagnostic review. No full build required until final integration.

#### Phase 8.2.6: Pass Declaration, Runtime, And PSO Architecture Cleanup

Work:

- Use the DLSS integration to reduce ad hoc pass insertion in frame assembly.
- Move toward high-level frame stages and feature modules that declare resources, schedule work, and expose final products through narrow hooks.
- Keep authored shader PSO handling separate from external SDK feature runtime handling. A DLSS provider should not require a fake shader package, fake PSO, or authored pass type.
- Preserve existing typed pass parameter validation for authored shader passes while adding equivalent contract validation for external providers.
- Remove or relocate provider-specific constants and resource decisions from general frame files as part of this phase; frame orchestration should remain stage scheduling plus product selection.

Acceptance criteria:

- Frame assembly reads as stage orchestration, not implementation detail.
- Shader pass runtime management and external provider runtime management are distinct code paths with parallel diagnostics.
- Feature modules can participate in scheduling without adding vendor fields to generic scene target structs.

Validation:

- Code review checklist only for this phase. No full build required until final integration.

#### Phase 8.2.7: NVIDIA DLSS Provider Implementation

Work:

- Integrate the selected NVIDIA SDK path behind the provider boundary.
- Initialize SDK state from RHI-provided native handles and renderer-provided application/version metadata.
- Query supported DLSS modes and recommended render extents from the SDK.
- Evaluate DLSS after opaque/transparent scene rendering and before UI/presentation, using the typed input contract and FrameGraph external-provider scheduling.
- Implement deterministic fallback when SDK initialization, capability query, feature creation, resource tagging, or evaluation fails.

Acceptance criteria:

- DLSS is disabled by default until SDK availability, resource contract, and settings are valid.
- DLSS quality mode and render/display extent choices come from SDK queries, not hard-coded guesses.
- All SDK calls are isolated to the provider implementation and a narrow platform/RHI bridge.
- Provider diagnostics include SDK version, backend, adapter, selected mode, input/output extents, reset state, and failure reason.

Validation:

- This is the first phase that requires a real build.
- Build targeted renderer/editor target, then run a no-DLSS fallback smoke and a DLSS-enabled smoke on supported hardware.

#### Phase 8.2.8: Production Quality Validation

Work:

- Add validation scenes and captures for static camera, camera pan, disocclusion, thin geometry, alpha-tested geometry, emissive/high-contrast content, transparent content policy, resize, quality-mode switch, and scene-cut reset.
- Add capture/repro instructions for DLSS artifacts without leaking SDK internals into global renderer state.
- Add backend parity notes for D3D12 and Vulkan. If a backend is not supported by the selected SDK path, report that explicitly.

Acceptance criteria:

- DLSS output, fallback output, reset behavior, and resize behavior have repeatable smoke coverage.
- Logs make DLSS provider state understandable without a debugger.
- Artifact reports can identify input contract violations versus SDK/provider failures.

Validation:

- Final validation only: targeted build, shader/cook validation if affected, runtime smoke, resize smoke, quality-switch smoke, and unsupported-backend fallback smoke.

Input contract:

- HUD-less scene color, depth, motion vectors, exposure/HDR metadata, camera jitter, render/display extents, frame index, and reset/camera-cut state must be available through a typed renderer contract.
- Motion vectors must match NVIDIA's expected space, scale, and sign for the selected SDK path; log the convention at startup and validate it in smoke scenes.
- Jitter must be owned by the temporal state system and exposed to DLSS without duplicating the pattern generator in the provider.
- Resize, quality-mode changes, device changes, DLSS toggles, scene cuts, and invalid history events must recreate or reset DLSS features deterministically.

Acceptance criteria:

- DLSS availability is reported through RHI/renderer capabilities with clear backend, adapter, driver, SDK, and reason-for-unavailable diagnostics.
- DLSS quality modes and dynamic-resolution behavior are queried from the SDK and only exposed when supported.
- DLSS evaluation is scheduled as an explicit renderer upscaler stage with declared inputs and output; fallback is deterministic when unavailable or disabled.
- FrameGraph diagnostics show the color/depth/motion/exposure inputs, output resource, external-provider barrier/state assumptions, and reset reason when applicable.
- The renderer can run with DLSS disabled without allocating DLSS SDK state or changing the non-DLSS pass graph unexpectedly.
- Validation includes static camera, camera pan, disocclusion, thin geometry, alpha-tested geometry, emissive/high-contrast content, resize, quality-mode switch, and scene-cut reset cases.
- Refactor gate: the implementation improves renderer architecture by separating feature-provider integration, pass orchestration, RHI capability reporting, and temporal contracts.

### Stage 8.3: Upscaler Hardening And DLSS Quality Validation

Goal: harden the DLSS integration until it is production-ready across supported backends, quality modes, and renderer workflows.

Implementation prompt:

```text
Add DLSS validation scenes, diagnostics, runtime toggles, fallback tests, and backend parity checks.
Use this stage to prove the architecture introduced in Stage 8.2 rather than adding new temporal algorithms.
```

Acceptance criteria:

- DLSS and fallback output paths are covered by targeted smoke tests where platform tooling allows.
- Runtime logs identify SDK/provider selection, quality mode, render/display extents, jitter, motion-vector convention, reset state, and resource tagging.
- Capture/debug flows preserve enough information to reproduce DLSS artifacts without exposing SDK internals as renderer-global state.

## Stage 9: NVIDIA NRD SIGMA Integration

Goal: Integrate NVIDIA NRD SIGMA as the production denoiser path for soft ray traced shadows.

This stage starts only after Stage 8.1/8.2/8.3 complete.

### Stage 9.1: NRD contract, contracts, and scheduling

Implementation prompt:

```text
Add denoiser contracts for resources, view-state inputs, and denoising modes.

Define what NRD needs for soft shadow denoising, how those resources are represented in FrameGraph, and how renderer-owned scheduling expresses `compute soft visibility` -> `NRD denoise` -> `consume denoised visibility`. Implement diagnostics for missing contracts and clear capability gating before any backend-specific callsites.
```

Suggested file additions:

- `Engine/Renderer/Public/Denoising/ShadowDenoiseContract.h`
- `Engine/Renderer/Private/Denoising/ShadowDenoiseContract.cpp`
- `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphDenoiserRegistration.*`

Acceptance criteria:

- FrameGraph exposes denoiser input, intermediate, and output resources for shadow visibility and history.
- Soft shadow path logs an explicit contract summary (motion vectors, depth, normals, jitter, history) before any NRD dependency runs.
- The engine can run with NRD off and still produce valid hard or raw soft visibility paths under clear mode selection.
- Missing contracts are treated as renderer diagnostics, not silent fallbacks in shader code.
- Refactor gate: NRD-specific terms are not introduced in RHI, GameFramework, cookers, or importer surfaces.

### Stage 9.2: NRD adapter and D3D12 path

Goal: ship the first working SIGMA integration using an explicit renderer adapter.

Implementation prompt:

```text
Introduce a Renderer-owned NRD adapter for D3D12 that consumes Sparkle shadow visibility inputs and emits denoised visibility output in a FrameGraph-owned resource graph.

Build an adapter boundary around NRD headers/libraries, translate Sparkle descriptors to NRD descs, and schedule SIGMA dispatches as normal renderer passes. Keep the denoiser path optional but explicit.
```

Suggested file additions:

- `Engine/Renderer/Private/Denoising/NrdDenoiserContext.h/.cpp`
- `Engine/Renderer/Private/Denoising/NrdSigmaShadowPass.h/.cpp`
- `Engine/Renderer/Private/Denoising/ShadowDenoiserPolicy.h/.cpp`
- `Engine/Renderer/Private/Denoising/ShadowDenoiserResources.h/.cpp`

Acceptance criteria:

- NRD is optional and CMake-gated with runtime-capability diagnostics.
- D3D12 can schedule and execute SIGMA denoising for soft directional and local shadows from FrameGraph resources.
- Direct lighting consumes denoised visibility and no NRD types leak into lighting passes.
- History behavior includes deterministic reset on resize, level switch, and denoiser-mode changes.
- Fixed one visibility sample per pixel remains preserved in the production soft-shadow path.
- Refactor gate: NRD integration is isolated in dedicated denoising files and does not add backend hacks in pass orchestration.

### Stage 9.3: Vulkan parity and temporal stability

Goal: complete NRD SIGMA integration readiness under parity and temporal stability expectations.

Implementation prompt:

```text
Implement Vulkan NRD SIGMA scheduling where backend support exists; otherwise keep explicit capability diagnostics and deterministic fallback behavior that is visible to users and logs.

Complete temporal inputs needed by NRD denoising (jitter, view-motion/history metadata, motion vectors when available) and validate convergence behavior with movement and resize paths.
```

Acceptance criteria:

- D3D12 and Vulkan share the same denoiser FrameGraph contract and scheduling model.
- If Vulkan lacks a required feature set, startup logs a clear skip/error and runs the allowed fallback mode by design.
- NRD SIGMA consumes and outputs FrameGraph-owned resources with no direct resource ownership in shader, pass-local, or GameFramework code.
- Camera cuts and level switches reset denoiser history deterministically.
- Motion/temporal inputs are present in the denoiser contract where available; missing inputs are logged explicitly.
- Refactor gate: temporal-state logic remains in renderer frame systems, not in denoiser internals or lighting shader code.

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

