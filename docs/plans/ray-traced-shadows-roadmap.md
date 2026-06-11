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

Every implementation step in this roadmap must include a cleanup/refactor slice. The feature is not considered done if it works by increasing coupling, hiding backend behavior, or making FrameGraph harder to reason about.

Per-step refactor checklist:

- Identify one renderer/RHI/FrameGraph responsibility that became clearer because of the step.
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

## Contract-first design (applies to all renderer feature steps)

To avoid hidden coupling, each pass should define a contract that is explicit in code:

- **Inputs**: resources it consumes (for example G-buffer, depth, normals, TLAS, motion vectors).
- **Outputs**: resources it produces or writes (for example visibility buffers, denoised visibility, lighting targets).
- **Assumptions**: required states or flags (for example TLAS build/read state, history validity, max light counts).
- **Ownership rules**: which subsystem creates and transitions each resource.

This roadmap uses contracts so feature code can be added by adding/expanding the relevant contract and then implementing against the declared interfaces.
A contract is not extra complexity by itself; it is a structure that makes ownership, ordering, and fallback behavior readable.

## FrameGraph Refactor Track

This track runs alongside every feature step.

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
- If a resource cannot be represented in FrameGraph yet, the step must add the missing FrameGraph concept instead of hiding the resource in pass code.

## Completed Foundation

Status: Source implemented; full build/runtime validation remains blocked by the local MSVC environment issue noted in earlier work.

The detailed historical prompts for the completed foundation have been removed from this living roadmap so the remaining document focuses on active architecture and next work. The completed foundation is:

- RHI ray tracing capability reporting, validation, AS resource descriptions, AS build commands, and shader metadata checks.
- FrameGraph support for ray tracing resources, external persistent resources, resource contract diagnostics, and compiled barrier playback.
- Renderer-owned ray tracing scene data: BLAS cache, TLAS build path, scene diagnostics, and FrameGraph integration for AS build/use ordering.
- Directional, point, and spot hard ray traced shadow visibility through shared shader helpers and renderer-owned settings.
- Ray traced shadow diagnostics and capability reports kept under renderer ray tracing code, not general frame orchestration.
- One-sample stochastic soft-shadow visibility signal generation with hard-shadow mode preserved.

Foundation rules that still apply:

- GameFramework owns authored scene/light data; Renderer owns render-scene ray tracing resources; RHI owns backend-neutral AS and native GPU contracts.
- FrameGraph owns pass ordering, resource lifetime, resource states, barriers, and diagnostics.
- Final lighting consumes visibility products. Shadow sampling and denoiser preparation remain separate from lighting composition.
- No backend-specific renderer shortcuts, shadow-map compatibility branch, imported-vs-authored shadow behavior, or GameFramework-owned AS state should be added.

## Active Plan: Temporal, Upscaling, DLSS, And Denoising

Goal: establish the temporal, external-feature, and upscaler architecture needed by DLSS, NRD SIGMA, and future renderer features without putting provider details in frame orchestration.

This section is the single source of truth for DLSS planning. Do not maintain a separate DLSS implementation record unless a later vendor SDK integration needs version-specific release notes.

## Step 1: Temporal Inputs And Motion Vectors

Status: Source implemented; build validation remains deferred with the surrounding renderer/RHI work.

Goal: provide temporal data required by shadow denoising, temporal upscaling, and history resets.

Contract:

- Motion vectors are available as renderer frame products.
- Temporal state owns previous view/projection, jitter, history validity, reset reason, and camera-cut detection.
- Consumers receive provider-neutral temporal summaries; they do not derive jitter or history state inline.
- Missing temporal inputs are logged and block incomplete temporal paths.

Validation:

- Renderer/RHI build when the current renderer changes are validated together.
- Runtime smoke for resize, level switch, camera cut, camera pan, and static camera history stability.

## DLSS Integration Rules

Goal: integrate NVIDIA DLSS as a production external upscaler provider while improving Sparkle's RHI capabilities, FrameGraph shape, pass declaration, runtime scheduling, and final-frame product architecture.

Implementation prompt:

```text
Integrate DLSS through NVIDIA Streamline first unless a backend constraint forces direct NGX.
Treat DLSS as a renderer-owned external feature provider, not as a Sparkle-authored shader pass.
Keep generic renderer/RHI/FrameGraph code provider-neutral; isolate DLSS, Streamline, NGX, SDK calls, tags, and handles inside the NVIDIA provider module and narrow RHI bridges.
```

Reference posture:

- Primary references: NVIDIA Streamline, Streamline DLSS guide, Streamline Sample, `vk_streamline`, and the NVIDIA DLSS programming guide.
- Inherit production practices rather than vocabulary: explicit resource contracts, SDK-owned provider lifetime, RHI-owned native handle exposure, FrameGraph-visible scheduling, deterministic fallback, and final product handoff.
- D3D12 and Vulkan are production targets. A backend may be unavailable only with a precise SDK/runtime/hardware/resource reason and a deterministic fallback.
- Runtime PSO handling remains for Sparkle-authored passes. DLSS evaluation is an external provider dispatch and must not require fake shaders, fake PSOs, or pass-local backend hacks.

Separation rules:

- RHI exposes vendor-neutral external-feature facts: backend API, adapter identity, native device/queue/command-list or command-buffer access, native resource support, explicit state control, and provider evaluation readiness.
- Renderer owns provider selection, SDK lifetime, quality mode, reset propagation, diagnostics, and fallback policy.
- FrameGraph owns declared reads/writes, scheduling point, resource state intent, external-provider barriers, and the selected `FinalSceneColor`.
- Presentation consumes `FinalSceneColor` and never branches on DLSS, Streamline, NGX, or quality mode.
- Frame assembly schedules high-level steps. It must not calculate DLSS constants, SDK tags, resource tagging, or backend-specific reset rules inline.

Layer ownership guide:

| Layer | Owns | Must not own |
| --- | --- | --- |
| RHI | Backend API identity, adapter identity, native handle availability, queue/command-list or command-buffer exposure, native resource exposure, explicit state support, backend bridge readiness diagnostics. | DLSS availability policy, Streamline/NGX headers, SDK feature handles, quality modes, resource tags, SDK binary lookup, provider fallback decisions. |
| Renderer upscaling subsystem | Provider selection, provider lifetime, provider-neutral settings, frame setup/reset notifications, diagnostics, fallback policy, final output product policy. | Backend object casts, direct D3D12/Vulkan object ownership, authored shader PSO workarounds, lighting/denoiser implementation details. |
| NVIDIA DLSS provider | DLSS capability interpretation, SDK/runtime integration, SDK feature lifetime, quality/render extent translation, resource tagging, per-frame constants, SDK evaluation, DLSS-specific diagnostics. | Generic frame state fields, generic RHI capability fields, lighting pass ownership, presentation branching. |
| FrameGraph | External-provider scheduling contract, declared reads/writes, resource states, barrier assumptions, final scene color handoff. | SDK calls, SDK tags, provider feature policy, backend-specific native handle casts. |

DLSS feature menu:

- Super Resolution: first shippable DLSS feature and baseline temporal upscaler.
- Native-resolution DLSS AA mode: DLSS at native render/output extent; no in-house native-AA shader path.
- Ray Reconstruction: reconstruction for explicit noisy ray-traced indirect diffuse/specular or path-traced signals.
- Frame Generation: generated intermediate frames, gated by swap-chain/present, hudless/UI, frame ID, latency, and pacing contracts.
- Multi Frame Generation and Dynamic Multi Frame Generation: SDK-supported generated-frame multipliers where hardware/runtime support exists.
- Model/preset and quality-mode management: queried from the SDK/runtime, not hard-coded.

Signal denoiser ownership:

- Direct ray-traced shadow visibility: NRD SIGMA-owned.
- Indirect diffuse: DLSS-RR-owned when its signal contract is valid; otherwise renderer fallback or selected non-DLSS path.
- Indirect specular/glossy: DLSS-RR-owned when its signal contract is valid; otherwise renderer fallback or selected non-DLSS path.
- Final lighting composition consumes resolved products and does not denoise.
- The same signal must not be routed through both NRD and DLSS-RR.

## Step 2: Reference Decision And Architecture Rules

Status: Complete; merged into this roadmap.

Output:

- Streamline-first is the default production path. Direct NGX remains an investigation fallback only if Streamline blocks a required backend, packaging, or feature requirement.
- Required provider inputs are identified: HUD-less scene color, output color, depth, motion vectors, optional exposure, jitter, render/output extents, frame index, reset state, and native command execution context.
- Ownership boundaries are defined in the DLSS integration rules above.

Validation:

- Documentation review only.

## Step 3: RHI Capability And External Feature Surface

Status: Source/API shape added; full build deferred.

Output:

- `RhiCapabilities` exposes a vendor-neutral external-feature interop surface.
- D3D12 and Vulkan bridge facts are populated separately while sharing the renderer-facing capability shape.
- Vulkan defaults to manual RHI-owned function-pointer hooking; the automatic Streamline interposer is a documented fallback only if manual hooking fails validation.
- Renderer startup has a DLSS capability summary path that does not create a DLSS feature instance.
- DLSS provider availability remains false until SDK binary lookup, runtime initialization, adapter support query, and feature support query exist.

Acceptance criteria:

- Backend native-handle readiness is separate from DLSS provider availability.
- No NVIDIA SDK fields, enums, handles, or headers appear in generic RHI capability structs.
- Native handle access stays behind RHI interfaces; renderer systems do not cast backend objects directly.
- D3D12 and Vulkan report enough adapter, device, queue, command-list/command-buffer, native resource, and runtime-check detail for the provider to decide availability.
- Missing Vulkan hook, extension, loader, queue, command buffer, or resource tagging requirements are reported as Vulkan DLSS capability failures, not renderer-wide failures.

Validation:

- Header/API review and diagnostic text review.
- Expected compile target when toolchain is available: `cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1`.

## Step 4: Renderer Upscaler Provider Boundary

Status: Source/API shape added; provider evaluation remains passthrough/stub until later steps add FrameGraph external-provider execution and the NVIDIA SDK runtime.

Work:

- Introduce a renderer-owned upscaler subsystem with providers: deterministic passthrough and NVIDIA DLSS.
- Define provider interfaces for initialization, capability query, frame setup, evaluation, resize/reset, shutdown, diagnostics, and fallback.
- Keep SDK lifetime, feature handles, quality mode, render extent, and reset handling inside provider implementation.

Output:

- Added a renderer-owned upscaler subsystem with a provider-neutral `IUpscalerProvider` interface.
- Added a deterministic passthrough provider.
- Added an NVIDIA DLSS provider stub under a dedicated `Upscaling/NvidiaDlss` folder. It consumes the DLSS capability report but owns no SDK state until the provider implementation step.
- Added `r.Upscaler.Provider` as the provider-neutral selection CVar: `0=Passthrough`, `1=NVIDIA DLSS`.
- Renderer startup owns provider selection and lifecycle through the upscaler subsystem without adding DLSS fields to `FrameContext`.
- RHI remains provider-neutral and exposes only external-feature interop facts; DLSS availability policy stays in the renderer provider layer.

Acceptance criteria:

- Lighting, shadow denoising, G-buffer, and presentation code do not branch on provider type.
- DLSS settings do not enter generic `FrameContext` except through provider-neutral render/output extent and final-product contracts.
- The same provider interface drives D3D12 and Vulkan.

Validation:

- Static dependency and include review.

## Step 5: Temporal And Resource Input Contract

Status: Source/API shape added; final output remains the native scene color until Step 6 adds a FrameGraph external-provider scheduling point and final scene color product.

Work:

- Define `UpscalerInputContract`: HUD-less scene color, depth, motion vectors, exposure/HDR metadata, jitter, render extent, display extent, frame index, reset state, camera-cut state, and final output target.
- Formalize motion-vector units, sign, jitter inclusion, viewport scaling, and depth convention for provider consumption.
- Keep temporal derivation in temporal/frame helpers, not in frame assembly or provider callsites.

Output:

- Added `UpscalerInputContract` for HUD-less scene color, depth, motion vectors, optional exposure, final output, render/output extents, frame index, temporal state, reset state, and provider-facing conventions.
- Added provider-neutral validation that reports missing required inputs and causes deterministic passthrough fallback for that frame.
- Formalized the current Sparkle motion-vector convention as pixel-space `CurrentMinusPrevious` values generated from jittered current and previous clip positions.
- Formalized the current depth input as device depth.
- Exposed the G-buffer motion-vector texture as a generic renderer frame product. Upscaling consumes it through the input contract, but it is not named or owned as an upscaler-only resource.

Acceptance criteria:

- Missing required inputs force deterministic fallback with precise diagnostics.
- Resize, quality change, scene cut, invalid history, device change, and provider toggle all flow into provider reset or recreation.

Validation:

- Contract review and diagnostic-path review.

## Step 6: FrameGraph External Provider Evaluation

Status: Source/API shape added; the evaluation node currently writes the deterministic passthrough fallback into `FinalSceneColor` until the DLSS provider runtime is integrated.

Work:

- Add FrameGraph concepts for external provider evaluation instead of modeling DLSS as an authored shader pass.
- Declare provider reads/writes, resource states, fallback output, and command-list or command-buffer execution requirements.
- Make presentation consume `FinalSceneColor`, regardless of whether native rendering, fallback, or DLSS produced it.

Output:

- Added `FinalSceneColor` as a generic renderer frame product, separate from HUD-less `SceneColor`.
- Added an `ExternalProvider` FrameGraph pass kind so provider scheduling is visible in pass labels, event scopes, GPU marker coloring, and FrameGraph diagnostics.
- Added labeled resource declarations for non-shader FrameGraph passes, allowing the provider boundary to declare `HudlessSceneColor`, `Depth`, `MotionVectors`, and `FinalSceneColor`.
- Added `EvaluateExternalUpscalerProvider` between lighting and presentation. It invokes the renderer upscaler subsystem through execution-time services and writes the deterministic fallback copy when the active provider does not produce output.
- Updated presentation and viewport products to consume `FinalSceneColor`.
- Kept provider implementation details outside `FrameContext`, lighting, denoiser, G-buffer, and presentation code. RHI remains responsible only for backend-native capability and handle bridges.

Acceptance criteria:

- Provider scheduling is visible in FrameGraph diagnostics.
- Provider evaluation cannot accidentally consume lighting internals, denoiser intermediates, editor UI, or HUD overlays.
- Non-DLSS frame graphs do not allocate provider-only resources.
- D3D12 and Vulkan use the same declared contract with backend-native handles resolved only by the provider/RHI bridge.

Validation:

- FrameGraph contract and diagnostic review.

## Step 7: Pass Declaration, Runtime, And PSO Cleanup

Work:

- Use the upscaler integration to move frame assembly toward high-level steps that declare inputs, outputs, and final products.
- Keep authored shader PSO management separate from external SDK runtime management.
- Add external-provider contract validation parallel to shader pass parameter validation.

Acceptance criteria:

- Frame assembly reads as orchestration, not implementation detail.
- Provider-specific constants and resource decisions live outside general frame files.
- Feature modules can schedule work without adding vendor fields to shared target structs.

Validation:

- Code review checklist before provider implementation.

## Step 8: NVIDIA DLSS Super Resolution Provider

Status: Provider/runtime architecture added; actual Streamline SDK calls remain unavailable until the SDK headers, binaries, and packaging path are added.

Work:

- Integrate Streamline-backed DLSS Super Resolution behind the provider boundary.
- Initialize SDK state from RHI native handles and renderer application metadata.
- Query supported quality modes and recommended render extents from the SDK.
- Evaluate after scene rendering and before UI/presentation through the FrameGraph external-provider scheduling point.
- Implement deterministic fallback when SDK initialization, capability query, feature creation, tagging, or evaluation fails.
- Implement D3D12 and Vulkan bridges through the shared provider contract.

Output:

- Added a DLSS runtime state model covering `NotSelected`, `Unavailable`, `AvailableNotCreated`, `Created`, `Evaluating`, and `FailedWithFallback`.
- Added provider-neutral quality mode settings with `r.Upscaler.QualityMode`.
- Extended the external-provider evaluation contract with backend API, native command-list/command-buffer handle, and native input/output/depth/motion-vector resources.
- Added a Streamline runtime adapter boundary under `Upscaling/NvidiaDlss`; SDK lifetime, feature creation, resource tagging, options, constants, and evaluation belong there, not in frame orchestration.
- Added a compiled unavailable Streamline runtime adapter so builds without the SDK deterministically report DLSS unavailable and fall back to passthrough.
- DLSS diagnostics now carry SDK/runtime version, backend, adapter, selected quality mode, render/output extents, reset state, runtime state, and failure reason.
- D3D12 and Vulkan still use the same provider contract; backend-specific SDK calls remain isolated to the future Streamline runtime implementation.

Acceptance criteria:

- DLSS is not selected until SDK availability, resource contract, hardware support, and user settings are valid.
- Provider diagnostics include SDK version, backend, adapter, selected mode, input/output extents, reset state, and failure reason.
- D3D12 and Vulkan both reach equivalent states: unavailable, available-not-created, created, evaluating, failed-with-fallback, and not-selected.
- A failure on one backend does not hide capability on the other.

Validation:

- First required build step: targeted renderer/editor build.
- No-DLSS fallback smoke and DLSS-enabled smoke on D3D12 and Vulkan where supported hardware and SDK runtime are available.

## Step 9: DLSS Feature Matrix And Native AA

Work:

- Query support for Super Resolution, native AA mode, Ray Reconstruction, Frame Generation, Multi Frame Generation, Dynamic Multi Frame Generation, latency hook requirements, quality modes, model/preset recommendations, and required resources.
- Add native-resolution DLSS AA mode by reusing the Super Resolution contract with render extent equal to output extent.

Acceptance criteria:

- Startup logs a D3D12/Vulkan feature matrix with independent states: unavailable, available, enabled, active, failed-with-fallback, and not-selected.
- Native AA can be selected without introducing custom native-AA shader ownership.
- Unsupported hardware keeps supported DLSS features available.

Validation:

- Feature-matrix review.
- Static camera, camera pan, sub-pixel geometry, and high-contrast edge validation.

## Step 10: Frame Pacing, Frame Generation, And MFG

Work:

- Add provider-neutral frame IDs, timing, present scheduling, frame pacing diagnostics, hudless/UI handling, and latency hooks required by generated frames.
- Add DLSS Frame Generation only after the swap-chain/present contract is explicit.
- Add Multi Frame Generation and Dynamic Multi Frame Generation through SDK-queried hardware/runtime limits.
- Do not make NVIDIA Reflex a standalone Sparkle feature; use SDK latency hooks only when the selected generated-frame path requires them.

Acceptance criteria:

- Generated-frame features are available only when SDK/runtime, backend bridge, hardware, latency, swap-chain, UI, and resource contracts are valid.
- Frame Generation teardown restores normal present behavior cleanly.
- MFG availability is reported separately from ordinary Frame Generation.

Validation:

- Frame pacing diagnostics review.
- D3D12 and Vulkan generated-frame smoke where supported, plus fallback smoke where unsupported.

## Step 11: Ray-Traced Indirect Diffuse And Specular Contracts

Work:

- Add renderer-owned contracts for noisy ray-traced indirect diffuse and indirect specular/glossy products before enabling DLSS-RR.
- Define guide buffers: diffuse albedo, specular albedo/reflectance, normal/roughness, linear depth, motion vectors, optional specular motion vectors or specular hit distance, and color-before-transparency where needed.
- Keep these as lighting products and FrameGraph resources, not DLSS-specific frame fields.

Acceptance criteria:

- Indirect diffuse and indirect specular/glossy products can be declared, scheduled, inspected, and consumed independently.
- Guide buffers have explicit format, resolution, lifetime, and state contracts.
- Direct shadow visibility is not routed through DLSS-RR.
- Lighting orchestration exposes products and ownership, not provider details.

Validation:

- FrameGraph contract review.
- Static diagnostic review that SIGMA owns direct shadow visibility and DLSS-RR owns only validated indirect signals.

## Step 12: DLSS Ray Reconstruction

Work:

- Add Ray Reconstruction as a separate feature contract for explicit ray-traced/path-traced signals.
- Require noisy indirect diffuse/specular ownership, guide buffers, linear depth, correct motion/specular motion data, normal/roughness, mip-bias policy, and denoiser ownership.
- Coordinate with Step 14 so SIGMA remains the direct shadow visibility owner.

Acceptance criteria:

- Ray Reconstruction cannot enable unless the effect contract disables conflicting denoisers for that signal.
- Ray Reconstruction cannot claim direct shadow visibility while SIGMA is selected for direct shadows.
- Missing signal inputs or guide buffers force fallback with diagnostics distinct from Super Resolution failures.

Validation:

- Static camera, camera pan, disocclusion, glossy/specular motion, and denoiser-off comparisons.

## Step 13: Production Quality Validation

Work:

- Add validation scenes and capture instructions for static camera, camera pan, disocclusion, thin geometry, alpha-tested geometry, emissive/high-contrast content, transparent content policy, resize, quality switch, scene-cut reset, UI overlay, generated frames, and fallback.
- Validate the full chosen feature matrix across D3D12 and Vulkan.

Acceptance criteria:

- DLSS output, fallback output, reset behavior, resize behavior, and provider teardown have repeatable smoke coverage.
- Logs identify provider, SDK/runtime state, quality mode, render/display extents, jitter, motion-vector convention, reset state, resource tagging, and fallback reason.
- Artifact reports can distinguish input contract violations from SDK/provider failures.

Validation:

- Targeted build, shader/cook validation if affected, D3D12 runtime smoke, Vulkan runtime smoke, resize smoke, quality-switch smoke, generated-frame smoke where supported, and backend-specific fallback smoke.

## Step 14: NVIDIA NRD SIGMA Integration

Goal: Integrate NVIDIA NRD SIGMA as the production denoiser path for direct ray traced shadow visibility.

This step starts only after the relevant temporal and upscaler contracts are complete.

Ownership split:

- SIGMA owns direct ray-traced shadow visibility.
- DLSS-RR owns indirect diffuse/specular reconstruction when its contract is valid.
- Final lighting composition consumes resolved products and does not denoise again.

## Step 15: NRD Contract And Scheduling

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

## Step 16: NRD Adapter And D3D12 Path

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

## Step 17: Vulkan Parity And Temporal Stability

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

## Step 18: Deforming Mesh And Animation Handling

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

## Step 19: Quality, Performance, And Many-Light Handling

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
- `SkeletalMorphTriangle` once Step 18 decides animated/deformable AS handling.
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

