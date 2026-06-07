# Ray Traced Shadows Roadmap

This plan introduces ray traced shadows in a Donut-inspired way while preserving Sparkle's existing module boundaries.

## Reference Model

- NVIDIA Donut keeps reusable render passes in `donut_render`, scene/runtime ownership in `donut_engine`, and API details behind NVRHI-style abstractions.
- Donut itself does not own ray tracing acceleration structures because AS requirements are application specific. It expects applications or samples to build AS data from scene representation and NVRHI abstractions.
- Donut-Samples includes `Ray Traced Shadows`, which rasterizes a G-buffer and renders basic ray traced directional shadows on DX12 and Vulkan.
- NVIDIA Real-Time Denoisers (NRD) is the correct NVIDIA integration target for denoised ray traced shadow visibility. SIGMA is the shadow-focused NRD method and should be treated as a renderer denoiser backend, not as GameFramework lighting state.
- RTXDI is a separate direct-light sampling system. It can become a later many-light strategy, but the first soft-shadow integration should not replace Sparkle's current light ownership or direct-lighting path with RTXDI.

Sparkle should follow the same spirit: keep ray tracing scene resources as renderer-owned render data, not GameFramework/importer state; keep RHI backend details behind shared RHI descriptions; keep the lighting pass composable rather than turning DirectLighting into a god pass.

## Current Sparkle Starting Point

- RHI already exposes ray tracing capabilities through `RhiRayTracingCapabilities`.
- RHI already has bottom-level and top-level acceleration structure build commands.
- RHI already has `RhiRayTracingGeometryDesc`, `RhiRayTracingInstanceDesc`, AS prebuild info, scratch/AS/instance buffer creation, and acceleration-structure shader parameter semantics.
- D3D12 has capability reporting and AS build plumbing.
- Vulkan ray tracing capability/build parity must be audited before enabling backend parity.
- Renderer `GPUMesh` already exposes `GetRayTracingGeometry()`.
- Deferred lighting already rasterizes a G-buffer and computes direct lighting in `DirectLightingPass`, which matches the Donut-Samples shadow flow well.
- Renderer now has an explicit ray tracing capability report and ray traced shadow policy resolver under `Engine/Renderer/Private/RayTracing`.
- Ray traced shadow controls live in renderer CVars (`r.RayTracedShadows`, quality, denoiser, normal bias, max distance), keeping feature policy out of GameFramework and import/cook code.

## Architecture Rules

- GameFramework owns scene lights and mesh components; it must not own BLAS/TLAS resources.
- Renderer owns render-scene ray tracing resources derived from immutable scene snapshots and uploaded GPU meshes.
- RHI owns backend-neutral AS descriptions, build commands, barriers, resource views, and shader-visible bindings.
- FrameGraph owns pass resource lifetime, pass ordering, and barriers once ray traced shadows consume AS resources.
- Shaders consume a scene acceleration structure and lighting/G-buffer data; they must not know about glTF/import/cooked scene internals.
- Start with hard shadows for directional lights only. Add point/spot and soft shadows as later extensions.
- Soft shadows are a mandatory final goal, not an optional quality experiment.
- The target soft-shadow path is exactly one visibility ray per shaded pixel. Do not add a multi-sample shadow control or brute-force ray-count scaling path in the current architecture.
- Soft shadows must be represented as one-sample stochastic visibility signals plus denoiser input/output resources. Do not blur final lighting as a shortcut.
- Directional, point, and spot soft shadows should share common ray-query and denoiser plumbing while keeping light-shape sampling in light-specific shader helpers.
- NRD integration belongs behind a renderer denoiser adapter with RHI-neutral resource declarations. Do not expose NRD types through GameFramework, importer, cooker, or scene metadata.
- Prefer inline ray queries in the existing compute direct-lighting path first if both D3D12 and Vulkan can support it cleanly. Add full ray tracing pipelines/SBT only if a later feature needs ray generation/miss/hit shaders.

## Stage 0: Capability And Backend Audit

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
- Renderer startup logs one ray tracing capability summary and one resolved ray traced shadow policy summary.

## Stage 1: Renderer Ray Tracing Scene

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

## Stage 2: FrameGraph Resource Integration

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

## Stage 3: Directional Ray Traced Shadow Visibility

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
- Directional lights with `CastShadow = false` remain unshadowed.
- If ray tracing is unavailable, direct lighting falls back to current unshadowed behavior with one concise diagnostic.
- No shadow-map compatibility layer is introduced in this stage.
- D3D12 runtime smoke shows visible/deterministic shadow contribution.
- Vulkan either validates the same path or emits a clear capability skip if Stage 0 proves missing support.

## Stage 4: Shadow Controls And Runtime Policy

Goal: Make ray traced shadows controllable without spreading feature flags through unrelated systems.

Implementation prompt:

```text
Add renderer-level ray traced shadow settings.

Expose a renderer feature policy with mode Off, Auto, and ForceRayTraced. Add quality mode Hard/SoftAreaLights, denoiser mode Off/NrdSigma, shadow ray bias, max ray distance, and diagnostic override controls. Keep light ownership in GameFramework and shadow evaluation policy in Renderer.
```

Acceptance criteria:

- `Auto` uses ray traced shadows only when backend support and TLAS are available.
- `Off` exactly matches current direct lighting output.
- `ForceRayTraced` fails loudly if backend support is missing.
- Settings are read by Renderer pass setup, not by GameFramework light classes.
- Soft-shadow and NRD settings resolve to a single policy object before pass setup; individual passes must not read CVars directly.
- The resolved policy always reports `raysPerPixel=1`; this is a core design target, not a tunable quality slider.

## Stage 5: Local Light Shadow Extension

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

## Stage 6: Stochastic Soft Shadow Visibility

Goal: Generate physically meaningful one-ray-per-pixel noisy soft-shadow visibility for directional, point, and spot lights before denoising.

Implementation prompt:

```text
Add stochastic ray traced soft-shadow visibility.

Represent light size explicitly in renderer-facing light data: angular diameter for directional lights, radius for point lights, and radius/cone policy for spot lights. Sample the appropriate light shape once per shaded pixel and trace one visibility ray against TLAS. Output a shadow visibility signal and hit-distance/confidence metadata suitable for denoising. Keep direct lighting evaluation separate from the raw visibility generation.
```

Acceptance criteria:

- Directional lights support angular soft shadows without treating the sun as a point light.
- Point and spot lights support finite-area soft shadows with range-aware ray distances.
- Every shaded pixel traces at most one shadow visibility ray in the current soft-shadow path.
- Light shape data is renderer-facing lighting data, not imported-vs-authored special behavior.
- The first stochastic output is noisy but physically interpretable and deterministic under fixed seeds.
- Hard-shadow mode still bypasses stochastic sampling and denoiser resources.

Suggested shader structure:

- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowSignals.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/RayTracedShadowDenoiserInputs.hlsli`

## Stage 7: NVIDIA NRD SIGMA Integration

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
- When NRD is unavailable, `SoftAreaLights + NrdSigma` falls back or fails according to renderer policy with a clear diagnostic.
- Direct lighting consumes denoised visibility, not NRD-owned resources directly.
- History reset works on resize, camera cut, level switch, and denoiser setting changes.

## Stage 8: Temporal Inputs And Motion Vectors

Goal: Provide the temporal data needed for stable soft-shadow denoising under camera and object motion.

Implementation prompt:

```text
Add renderer temporal inputs required by NRD.

Track previous view/projection, camera jitter, per-pixel motion vectors, history validity, and responsive resets. Add object motion for moving/skinned meshes once animation ray tracing policy exists. Keep temporal resource ownership in FrameGraph and renderer frame state.
```

Acceptance criteria:

- NRD history resets deterministically on level switch and resize.
- Static camera soft shadows converge without visible instability.
- Moving camera soft shadows remain stable using camera motion vectors.
- Animated or deforming shadow casters either provide object motion or emit a clear reduced-quality diagnostic.

## Stage 9: Deforming Mesh And Animation Policy

Goal: Decide how animated skeletal/morph meshes participate in acceleration structures without muddying static mesh paths.

Implementation prompt:

```text
Add explicit ray tracing geometry update policy for skeletal and morphing meshes.

Keep static BLAS stable. For skeletal/morphing meshes, choose either CPU-deformed mesh upload reuse, refit/rebuild BLAS per geometry change, or exclude animated meshes from shadow casters with diagnostics until a scalable path exists.
```

Acceptance criteria:

- Static meshes do not pay skeletal/morph AS rebuild cost.
- Skeletal/morph meshes have explicit diagnostics for included, rebuilt, refit, or excluded state.
- Renderer does not infer AS update behavior from importer data.

## Stage 10: Quality, Performance, And Many-Light Policy

Goal: Improve quality and scalability after hard and denoised soft shadows are correct.

Implementation prompt:

```text
Add quality/performance policy for denoised soft shadows.

Tune blue-noise/sample sequence quality, checkerboard/half-res options if needed, light caps, tile classification, and local-light shadow budgeting while preserving the one-ray-per-pixel target. Consider RTXDI only as a later direct-light sampling system if the number of shadowed local lights outgrows simple bounded loops.
```

Acceptance criteria:

- Hard-shadow path remains available and deterministic.
- Soft-shadow and denoiser settings are renderer-owned.
- Denoising resources are FrameGraph-owned.
- Performance and quality tradeoffs are measured before claiming a win.
- Local-light shadow budgeting remains explicit when many of the 512 lights cast shadows.

## Validation Matrix

Minimum commands:

```powershell
cmake --build build-codex-ninja --target ShowcaseRuntime ShowcaseEditor ShaderCompiler -- -j 1
```

Required smokes:

- D3D12 runtime smoke with a simple occluder/receiver scene and one shadow-casting directional light.
- Vulkan runtime smoke for the same scene or a clear capability skip with diagnostic proof.
- Existing unshadowed scenes with ray traced shadows disabled.
- `CameraCubeImportedCamera` or another imported-light scene to prove glTF/imported lights use the same light path.
- `SkeletalMorphTriangle` once Stage 6 decides animated/deformable AS policy.
- A directional soft-shadow scene with large penumbra and camera motion.
- A local-light soft-shadow scene with point and spot lights at different radii/ranges.
- NRD disabled, unavailable, and enabled modes.

Log evidence to require:

- Backend ray tracing capability summary.
- BLAS build/reuse count.
- TLAS instance count.
- Shadow mode selected: Off, Auto fallback, or RayTraced.
- Shadow quality selected: Hard or SoftAreaLights.
- Denoiser selected: Off or NrdSigma, plus whether the NRD backend initialized.
- DirectLighting pass binding includes TLAS only when the selected mode requires it.
- Denoiser pass resources include visibility, depth, normal, motion/history, and output visibility when NRD is enabled.

## Non-Goals For First Changelist

- No path tracing.
- No ray traced reflections.
- No soft shadows in the first hard-shadow changelist. Soft shadows are a dedicated follow-up track with NRD integration.
- No RTXDI/ReSTIR.
- No GameFramework-owned AS state.
- No imported-vs-authored shadow behavior branches.
- No backend-specific renderer shortcuts hidden behind `if D3D12` in pass code.

## Open Design Questions

- Should first implementation use inline ray query inside `DirectLighting.hlsl`, or a separate shadow mask compute pass that DirectLighting samples?
- Should TLAS be rebuilt every frame initially for simplicity, then optimized, or should static/dynamic partitioning be introduced immediately?
- How much Vulkan ray tracing support is already implemented versus only described by RHI types?
- Should alpha-tested geometry be opaque in first pass, or should any-hit/alpha testing be deferred until material opacity data is bound for ray tracing?
- Which NRD package version should be vendored, and should it live under a CMake option such as `SPARKLE_ENABLE_NRD`?
- Should point/spot soft shadows be capped per frame independently from lighting count to avoid tracing and denoising too many of the 512 punctual lights?
- Do we need shadow visibility atlases/tiled denoising for many local lights, or is one screen-space aggregate visibility signal enough for the first NRD SIGMA integration?
