# PBR Implementation Action Plan

Date: 2026-06-30

This document is a staged implementation prompt plan. It starts from `Docs/Rendering/PBR/02-Sparkle-PBR-State-Audit.md` and aims at the requirements in `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md`.

Scope:

- Direct lighting
- Indirect diffuse/specular ray-traced lighting
- Distant sky texture fallback
- NVIDIA denoiser/provider readiness

Out of scope for this plan:

- Volumetric lighting
- ReSTIR
- DDGI/probes
- Neural radiance caches
- Full material model expansion beyond the current metallic-roughness surface

Every stage has two equal acceptance surfaces:

- physical correctness
- code structure and pass architecture

A stage is not complete if the output looks correct but the implementation makes the next lighting problem harder to solve.

Every stage also has a reference-proof requirement:

- The implementation prompt must include `Reference lineage`.
- Each nontrivial algorithm must cite a concrete repository, SDK sample, engine implementation, specification, or paper.
- If the code intentionally deviates from the reference, the prompt must name the difference and the concrete implementation invariant or architecture boundary that keeps the deviation from spreading.
- Final implementation files must not contain `Reference lineage` banners. References are tracked in prompts, design docs, audit docs, and PR notes; source comments are only for local non-obvious implementation details.
- Acceptance criteria must include a reference-compliance check.
- A feature without a credible external reference is experimental and cannot be marked PBR/reference-correct.

Every stage also has a reuse-proof requirement:

- The implementation prompt must include a `Reuse/DRY audit`.
- Before adding new logic, scan existing shader modules, pass classes, bindings, settings, CVars, shader registrations, and docs for equivalent bodies.
- Prefer reusing, moving, or generalizing existing bodies over adding near-copies.
- Generic utility logic must not live in feature-specific files. Color math, encoding, sampling, geometry, ray offsets, reductions, and provider/signal helpers belong in concept-owned reusable modules such as `Common/*`, `Lighting/*`, `RayTracing/*`, or narrowly named `Display/*` includes.
- New or touched shader registrations must use one source file per shader package registration unless a specific grouped-registration exception is documented and accepted.
- Feature enum families must live in focused type headers instead of accumulating in umbrella settings headers.
- Pass files must stay thin: generic parameter metadata, dispatch sizing, scheduling, binding, and frame-graph/RHI mechanics belong in pass/core utilities or the frame-graph/RHI layer.
- Do not add wrappers that only rename or forward parameters. Keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.
- If duplicate-looking code is intentionally kept, explain whether it is backend-specific, temporary, performance-motivated, or required by a real ownership boundary.
- Acceptance criteria must include a no-unjustified-functional-duplicate check.

## Stage 0: Lock the Lighting Contract

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Document and assert that direct and indirect lighting targets store material-evaluated outgoing radiance contributions in linear HDR. Add comments near target declarations and composite code. Update or supersede older docs that describe `IndirectDiffuse` as raw irradiance.

Reference lineage:

- PBRT rendering equation and path-tracing target: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- Google Filament PBR/color contract: <https://google.github.io/filament/Filament.md.html>
- Unreal Engine physically based materials: <https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine>
- Unreal Engine path tracer as reference-output concept: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>

Files:

- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl`
- `Engine/Renderer/Private/Frame/Lighting/LightingRenderTargets.cpp`
- Existing architecture docs that still describe old target semantics.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- `DirectDiffuse`, `DirectSpecular`, `IndirectDiffuse`, and `IndirectSpecular` have a written semantic contract.
- No doc claims `IndirectDiffuse` is raw irradiance unless the code is changed to match.
- `LightingComposite` remains a simple sum only because all inputs share the same radiance-contribution unit.
- Reference compliance: the final contract explicitly maps each target back to the rendering equation and names any deviation from PBRT/Filament/Unreal terminology.
- Reuse/DRY: no duplicate lighting-target semantic descriptions remain in other docs with conflicting wording.
- Source hygiene: no final shader/pass source file adds `Reference lineage` banners; references are held in this stage note, implementation prompt, audit note, or PR note.

Completion note:

- Reference lineage: implemented against the PBRT rendering-equation target, Filament linear HDR/material-evaluated color contract, Unreal physically based material terminology, and Unreal path tracer reference-output concept listed above.
- Reuse/DRY audit: scanned existing lighting target declarations, `LightingComposite`, PBR docs, and older indirect-diffuse architecture docs before editing. `Docs/Rendering/PBR/01-PBR-Reference-Requirements.md#lighting-target-contract` is the authoritative contract; older docs now point to or defer to it instead of carrying conflicting raw-irradiance semantics.
- Source hygiene: final shader/pass source comments describe only local buffer semantics and composite invariants; reference lineage stays in docs.

## Stage 0A: Lock Shader Module Boundaries

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Before changing lighting math, move reusable lighting concepts out of pass-specific shader folders. Do this as behavior-preserving refactors with shader-cook checks after each step.

Reference lineage:

- Unreal Engine shader development and modular shader organization: <https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine>
- NVIDIA Falcor render-pass and shader-module organization: <https://github.com/NVIDIAGameWorks/Falcor>
- NVIDIA RTX Path Tracing SDK modular path-tracing shaders: <https://github.com/NVIDIA-RTX/RTXPT>
- AMD FidelityFX SDK pass/kernel separation: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>

Target dependency direction:

```text
Common -> Geometry -> Material -> BRDF -> Lighting -> RayTracing -> Passes
```

Files to create or move toward:

- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/Lighting/Visibility.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSignals.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`

Move candidates:

- light direction/falloff/cone helpers from `DirectLightingCommon.hlsli` to `Lighting/PunctualLights.hlsli`
- direct surface lobe evaluation from `DirectLightingCommon.hlsli` to `Lighting/SurfaceLighting.hlsli`
- shadow signal/sampling helpers from `Passes/Deferred` to `RayTracing/Shadows`
- duplicated indirect surface records into `RayTracing/PathSurface.hlsli`

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- No file under `Engine/Assets/Shaders/RayTracing` includes `Passes/Deferred/*`.
- Generic lighting helpers do not live under `Passes/Deferred`.
- Deferred pass entrypoint files continue compiling and producing identical output after each move.
- `DirectLighting.hlsl`, `IndirectDiffuse.hlsl`, and `IndirectSpecular.hlsl` become thinner, not larger.
- The shader include graph has no cyclic conceptual dependency.
- Reference compliance: the final module layout names which Unreal/Falcor/RTXPT/FidelityFX patterns were followed and which local deviations were intentional.
- Reuse/DRY: moved helpers have exactly one canonical home; no pass keeps a forked copy of light falloff, direct-surface evaluation, path surface records, or shadow signal packing.
- Source hygiene: generic helpers live in concept-owned include files, not in first-use pass files, and final source files contain no `Reference lineage` banners.

Completion note:

- Reference lineage: followed Unreal/Falcor-style separation between pass entrypoints and reusable shader modules, RTXPT-style shared ray/path helper ownership, and FidelityFX-style pass/kernel boundary discipline.
- Local deviation: existing deferred pass entrypoints still own pass resources, debug selection, and dispatch shape; later stages will split path sampling and path lighting more deeply.
- Reuse/DRY audit: scanned existing direct-light, ray-hit, shadow, and indirect path bodies before moving logic. `Lighting/PunctualLights.hlsli`, `Lighting/SurfaceLighting.hlsli`, `RayTracing/PathSurface.hlsli`, and `RayTracing/Shadows/*` are now the canonical homes for moved light falloff/cone helpers, direct surface evaluation, path surface records, and shadow signal/sampling/trace helpers.
- Validation: cooked the affected direct-lighting and indirect-lighting shader packages after the moves.

## Stage 0B: Make Pass Entrypoints Thin

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Refactor direct and indirect pass entrypoints so they own pass IO and output policy only. Sampling, light evaluation, visibility tracing, hit/miss resolve, and path throughput should live in reusable modules.

Reference lineage:

- NVIDIA Falcor render pass boundaries and pass-resource ownership: <https://github.com/NVIDIAGameWorks/Falcor>
- NVIDIA RTX Path Tracing SDK separation between path state, sampling, and pass entrypoints: <https://github.com/NVIDIA-RTX/RTXPT>
- AMD FidelityFX SDK pattern of small dispatch kernels over shared helper code: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>
- Unreal Engine shader development guidance for permutation/pass ownership: <https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine>

Target pass entrypoint shape:

```text
main:
    guard dispatch bounds
    load GBuffer / constants
    early out sky pixels
    build primary surface
    call one reusable lighting/path function
    choose debug or production output
    write target
```

Refactor targets:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Pass entrypoint files do not define reusable BRDF, light falloff, ray-origin, lobe sampling, or path-throughput algorithms.
- Shared algorithms are named after concepts, not effects.
- Effect-specific inspection helpers stay pass-specific only when they cannot be expressed through an existing owned signal or output.
- Line count is reduced for `IndirectDiffuse.hlsl` and `IndirectSpecular.hlsl` by moving cohesive logic, not by hiding unrelated code in one new god include.
- New files each have one reason to change.
- Reference compliance: pass boundaries are compared against Falcor/RTXPT/FidelityFX patterns in the implementation notes.
- Reuse/DRY: entrypoints call shared concept modules for any repeated light, surface, sampling, tracing, or throughput logic.
- Registration hygiene: new or touched shader registrations use one source file per shader package registration.
- Settings hygiene: new or touched settings enums live in focused type headers, not umbrella settings files.

## Stage 0C: Display Transform and Exposure Metering

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Keep lighting buffers linear HDR and move display conversion into a dedicated presentation/display path. Exposure must be a frame-graph resource. Implement a production metering path with parallel reduction and an explicit alternate downsample-pyramid metering path. Automatic exposure must adapt temporally through persistent exposure history.

Reference lineage:

- AMD FidelityFX FSR2 luminance pyramid: <https://github.com/GPUOpen-Effects/FidelityFX-FSR2>
- AMD FidelityFX SPD downsampler: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>
- NVIDIA Falcor ToneMapper pass: <https://github.com/NVIDIAGameWorks/Falcor>
- Microsoft MiniEngine exposure/luma and eye-adaptation shaders: <https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine/Core/Shaders>
- Google Filament color/display management: <https://google.github.io/filament/Filament.md.html>

Files:

- `Engine/Assets/Shaders/Display/ToneMapping.hlsli`
- `Engine/Assets/Shaders/Passes/PostProcessing/Exposure*.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/OutputEncoding.hlsl`
- `Engine/Renderer/Private/Frame/PostProcessing/Exposure.*`
- `Engine/Renderer/Private/Passes/PostProcessing/ExposurePass.*`
- `Engine/Renderer/Private/Frame/Presentation/ToneMapping*.{h,cpp}`
- `Engine/Renderer/Private/FramePipeline/FramePipeline.*`
- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`

Required design:

- Production path: reduce log-luminance moments with groupshared compute reduction.
- Alternate path: 2x2 downsample pyramid over the same moments.
- Resolve path: select exactly one metering source from renderer settings, compute target exposure, and adapt automatic exposure against the previous frame in EV/log2 space.
- History path: keep previous/current exposure as persistent 1x1 frame-graph textures, reset history on camera cuts, temporal-history invalidation, resize, and scene-extent changes.
- Presentation path: sample only final scene color and exposure, then apply one tone mapper and one output encoding policy.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Reinhard, ACES approximate, and ACES fitted filmic operators are selectable.
- Exposure mode supports manual and automatic.
- Exposure metering supports parallel reduction and an explicit downsample pyramid mode.
- Automatic exposure has separate adaptation speed-up and speed-down renderer settings.
- The exposure texture stores adapted exposure, average luminance, target exposure, and previous exposure for debug visibility.
- Output encoding is explicit for linear UNorm and sRGB backbuffers.
- Implementation prompt, design notes, audit notes, or PR notes name the reference repositories above; final shader/source files do not use `Reference lineage` banners.
- Shader cook passes for DXIL and SPIR-V.
- Reference compliance: implementation notes map each exposure/tone-mapping subsystem to the exact reference path it follows.
- Reuse/DRY: reduction and downsample paths share the same moment payload and final resolve; duplicate-looking kernels are removed or justified by genuinely different sampling work; persistent texture binding is a frame-graph capability, not local exposure-pass machinery.

Completion note:

- Reference lineage: the production metering path follows the parallel log-luminance reduction shape used by real-time exposure systems such as Microsoft MiniEngine and the luminance-reduction side of AMD FSR2; the alternate metering path follows the 2x2 luminance-pyramid/downsample organization used by AMD FSR2/FidelityFX SPD. Exposure resolve follows the MiniEngine/Falcor pattern of resolving metering into a persistent adapted exposure, with the local deviation that Sparkle stores adapted exposure, average luminance, target exposure, and previous exposure together in one debug-visible `RGBA32F` exposure texture. Presentation follows Falcor's dedicated ToneMapper pass boundary and Filament's display-management split by keeping lighting buffers linear HDR, applying a single selected tone mapper, then applying one explicit output-encoding policy. Local deviation: Sparkle writes an encoded UNorm intermediate and copies it to the swapchain, so automatic output encoding currently selects shader sRGB bytes for both linear UNorm and sRGB backbuffer formats instead of relying on render-target sRGB write conversion.
- Reuse/DRY audit: scanned the existing exposure shaders, tone-mapping/display includes, presentation passes, frame-graph resource creation, frame-pipeline history binding/reset, renderer settings, and editor settings panel before adding logic. The reduction and pyramid paths share the same `float2(logLuminanceSum, sampleCount)` moment payload and the same exposure resolve shader; no duplicate resolve/adaptation code was introduced. Persistent previous/current exposure textures remain frame-graph resources, with `FramePipeline` only binding/resetting them when temporal history is invalidated.
- Validation: rebuilt `ShaderCompiler` so the cook tool picked up the current renderer shader registrations, then cooked `ExposureReduceScene`, `ExposureReduceTexture`, `ExposureDownsampleScene`, `ExposureDownsampleTexture`, `Exposure`, `ToneMapping`, and `OutputEncoding` for `DxilSm66` and `SpirV16`.

## Stage 0D: Audit and Refine Existing Exposure/Tone Mapping

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Review the currently implemented exposure and tone-mapping code against this document before treating it as production-ready. The implementation already exists, so this stage is a hardening/audit pass: check reference fidelity, color science assumptions, settings ABI, shader layout, frame-graph resource ownership, performance cost, validation scenes, and debug visibility.

Reference lineage:

- AMD FidelityFX FSR2 luminance pyramid: <https://github.com/GPUOpen-Effects/FidelityFX-FSR2>
- AMD FidelityFX SPD downsampler: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>
- NVIDIA Falcor ToneMapper pass and auto-exposure workflow: <https://github.com/NVIDIAGameWorks/Falcor>
- Microsoft MiniEngine `ExtractLumaCS` and exposure/luma shaders: <https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine/Core/Shaders>
- Google Filament color management and tone-mapping discussion: <https://google.github.io/filament/Filament.md.html>
- ACES fitted tone mapping references used by real-time engines must be named in implementation prompts, audit notes, PR notes, or docs where the display transform is specified.

Files:

- `Engine/Assets/Shaders/Display/ToneMapping.hlsli`
- `Engine/Assets/Shaders/Passes/PostProcessing/Exposure*.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/OutputEncoding.hlsl`
- `Engine/Renderer/Private/Frame/PostProcessing/Exposure.*`
- `Engine/Renderer/Private/Passes/PostProcessing/ExposurePass.*`
- `Engine/Renderer/Private/Frame/Presentation/ToneMapping*.{h,cpp}`
- `Engine/Renderer/ShaderRegistrations/Exposure*.cpp`
- `Engine/Renderer/ShaderRegistrations/ToneMapping*.cpp`
- `Engine/Renderer/ShaderRegistrations/OutputEncoding*.cpp`
- `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`
- `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`
- `Config/DefaultEngine.ini`

Audit checklist:

- Verify C++ `ToneMappingUniformData` layout exactly matches all HLSL constant buffers.
- Verify output encoding policy handles linear UNorm and sRGB backbuffers without double encoding.
- Verify exposure metering works on non-power-of-two extents without border overweighting.
- Verify reduction and downsample averages match within documented tolerance on deterministic HDR test images.
- Verify manual exposure bypasses automatic luminance selection but still applies compensation and min/max clamps as intended.
- Verify automatic exposure adapts in EV/log space using frame delta, separate speed-up/speed-down settings, and a valid-history bit.
- Verify exposure history resets on temporal-history invalidation, resize, scene extent changes, and camera cuts.
- Verify ACES approximate and fitted filmic operators are display-only and never affect lighting inputs.
- Verify generic color math, output encoding, exposure metering math, and reduction helpers live in concept-owned includes rather than in `ToneMapping.hlsli` or pass entrypoint files.
- Verify every constant/curve and exposure algorithm has a named reference in docs, implementation prompts, audit notes, or PR notes.
- Verify final implementation files contain no `Reference lineage` banners.
- Verify exposure and presentation shader registrations are one package registration per source file, or document an approved exception.
- Verify display/exposure enum families live in focused display type headers rather than the umbrella rendering settings state header.
- Verify exposure pass files contain pass orchestration only; generic metadata, dispatch, and frame-graph/RHI mechanics live in shared pass/core helpers.
- Verify downsample-pyramid metering is an explicit selectable path, not an implicit fallback or hidden validation mode.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Reference compliance: each tone curve, exposure metering path, temporal adaptation rule, history reset rule, and output encoding decision has a named AMD/NVIDIA/Microsoft/Filament/ACES reference or a documented deviation.
- Reuse/DRY: no duplicated constant buffer layouts, exposure formulas, adaptation formulas, output encoding formulas, generic color utilities, frame-graph persistent binding helpers, or settings enum mappings exist without a single canonical helper or documented reason.
- Functional validation: reduction and downsample metering agree within tolerance on black, gray, HDR ramp, HDR sky, and non-power-of-two test images.
- Presentation validation: scene color is tone mapped exactly once and sky/environment radiance remains HDR before presentation.
- Performance validation: cost of building both metering paths is measured; if too expensive, the validation path is made optional without invalidating the production path.
- Shader cook passes for all exposure and presentation packages on DXIL and SPIR-V.
- Documentation records what was audited and what remains non-production-grade.
- Source hygiene: final source has no `Reference lineage` banners, no touched registration hub for exposure/presentation shaders, no display enum dump in umbrella rendering settings, and no feature-owned copy of generic compute dispatch helpers.

Completion note:

- Reference lineage: reduction and pyramid metering were audited against AMD FidelityFX FSR2 luminance-pyramid structure, AMD FidelityFX SPD-style 2x2 downsample work decomposition, and Microsoft MiniEngine `ExtractLumaCS`/eye-adaptation style log-luminance reduction. Temporal adaptation follows MiniEngine/Falcor-style prior-frame exposure history, with Sparkle adapting in EV/log2 space through `Exposure::AdaptExposure`. Tone mapping follows Filament/Falcor's display-only presentation boundary; Reinhard is retained as a simple baseline curve, ACES approximate is the Krzysztof Narkowicz `ACESFilm` curve (`2.51, 0.03, 2.43, 0.59, 0.14`), and ACES fitted filmic uses the Stephen Hill / MJP BakingLab fitted RRT+ODT form and matrices. Output encoding follows Filament-style explicit display encoding, with a documented local deviation from PBR-R-013 hardware sRGB writes: Sparkle's current compute presentation path writes an encoded UNorm intermediate and copies it to the swapchain, so automatic mode selects shader sRGB bytes for both linear UNorm and sRGB backbuffer formats to avoid copy-path double conversion assumptions.
- Reuse/DRY audit: scanned exposure shaders, display includes, presentation passes, shader registrations, settings/CVars, `Config/DefaultEngine.ini`, frame-graph exposure resource ownership, frame-pipeline persistent history binding/reset, and sky/environment sampling before changing logic. Generic exposure math remains in `Display/Exposure.hlsli`; output encoding remains in `Display/OutputEncoding.hlsli`; tone curves remain in `Display/ToneMapping.hlsli`; pass entrypoints only bind resources, perform bounds checks, and call the concept helpers. Exposure and presentation shader registrations remain one package registration per source file. Display enums remain in `EngineRenderingDisplayTypes.h`, not the umbrella settings state header.
- Functional validation: deterministic CPU audit of the exact `float2(logLuminanceSum, sampleCount)` payload showed parallel reduction and 2x2 downsample-pyramid metering agree within floating-point noise: black `4x4` relative diff `0`, gray `4x4` `7.71e-16`, HDR ramp `8x4` `2.22e-16`, HDR sky `8x5` `0`, non-power-of-two checker `7x5` `2.15e-16`. The non-power-of-two case preserved `35` samples, confirming border pixels are not overweighted.
- Presentation validation: `Lighting/SkyEnvironment.hlsli` now returns sampled environment radiance without tone mapping, so the sky pass and indirect miss paths keep HDR radiance until the dedicated presentation tone-mapping pass. `Passes/Presentation/ToneMapping.hlsl` samples final scene color plus exposure and applies exactly one selected tone mapper; `Passes/Presentation/OutputEncoding.hlsl` applies the selected output encoding after tone mapping.
- Performance validation: only the selected metering path is built in the frame graph. Static work-size audit for representative extents measured the production reduction path at `3` passes / `8,201` intermediate texels for `1920x1080`, `3` / `14,461` for `2560x1440`, and `3` / `32,536` for `3840x2160`; the downsample-pyramid path measured `11` / `691,302`, `12` / `1,228,844`, and `12` / `2,764,902` respectively. The pyramid remains an explicit selectable validation/alternate path, not something built alongside production reduction.
- Remaining non-production-grade items: the output-encoding path still uses compute plus copy instead of a render-target sRGB write path for sRGB swapchains; runtime GPU timing captures for both metering methods still need to be added to the later validation/review pack; HDR environment import/calibration and sky reference captures remain later-stage work.
- Validation commands: rebuilt `ShaderCompiler`; cooked `ExposureReduceScene`, `ExposureReduceTexture`, `ExposureDownsampleScene`, `ExposureDownsampleTexture`, `Exposure`, `ToneMapping`, `OutputEncoding`, `Sky`, `IndirectDiffuse`, and `IndirectSpecular` for `DxilSm66` and `SpirV16`; rebuilt `ShowcaseEditor` DevelopmentEditor.

## Stage 0E: Repository-Wide PBR Council Audit Gate

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Before implementing a lighting phase, run a repo-wide audit across shaders, renderer frame graph, RHI formats, asset import/cooking, provider contracts, and settings. The audit must update this plan when it finds a PBR-relevant issue. This gate exists so implementation work cannot focus only on the active shader file while missing a format, import, denoiser, or provider assumption elsewhere.

Reference lineage:

- NVIDIA RTXPT renderer structure and correctness expectations: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor render-pass and path-tracing reference organization: <https://github.com/NVIDIAGameWorks/Falcor>
- NVIDIA NRD signal contracts: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA Streamline DLSS Ray Reconstruction provider-resource tagging: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- AMD FidelityFX SDK provider-style render-resource and temporal examples: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>
- Google Filament material, color, and lighting documentation: <https://google.github.io/filament/Filament.md.html>
- glTF 2.0 and `KHR_lights_punctual`: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>, <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual>
- Unreal Engine physically based materials, physical light units, and path tracer documentation.

Required scan categories:

- Shader modules: BRDF, material decode, GBuffer, direct lighting, indirect lighting, ray tracing, shadows, sky, presentation.
- Renderer passes: frame scheduling, pass data builders, frame-graph resource declarations, shader registrations, settings/CVars.
- RHI and formats: scene color, GBuffer, depth, motion vectors, denoiser signal resources, backbuffer encoding, HDR formats.
- Assets/tools: glTF material/light import, texture color-space policy, compression policy, HDR sky import, packed texture channels.
- Providers: DLSS, future DLRR, NRD SIGMA, indirect denoiser/reconstruction contracts, history/reset state.
- Correctness surfaces: representative scenes, pre-tonemap outputs, reference-mode outputs, and existing metrics when they already exist.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- The audit output maps every found issue to `file(s)`, `symptom`, `risk`, `required stage`, and `reference(s)`.
- No P0/P1 issue is left as prose-only documentation; it must be represented by a stage or by an explicit non-goal/deviation.
- The staged plan names the references inside the relevant stage prompt, not only in this global section.
- Reference compliance: every new stage added by the audit cites at least one NVIDIA/AMD/Epic/Filament/glTF/PBRT reference and states what part of that reference is being followed.
- Reuse/DRY: before proposing a new helper, resource, pass, or provider interface, the audit records the existing bodies that were scanned and whether they can be reused or unified.

Completion note:

- Reference lineage: the audit used RTXPT and Falcor for renderer-wide pass/resource/reference-output organization, NRD for denoiser signal contracts, Streamline DLSS Ray Reconstruction for provider-resource tagging expectations, AMD FidelityFX for provider-style temporal resource handling, Filament and Unreal for material/light/color expectations, glTF 2.0 plus `KHR_lights_punctual` for asset import and light-unit contracts, and PBRT for path-traced reference-output expectations already named in later stages.
- Reuse/DRY audit: scanned shader concept folders and entrypoints (`Material`, `BRDF`, `Lighting`, `RayTracing`, `Passes/Deferred`, `Passes/Presentation`), renderer frame data/builders, frame-graph resource declarations, GBuffer formats, exposure/presentation settings, DLSS/upscaler provider contracts, shadow denoiser contracts, glTF material import and texture cooking, and default HDR sky cooking before proposing follow-up work. Existing canonical bodies to reuse are `SurfaceLighting.hlsli`, `PunctualLights.hlsli`, `PathSurface.hlsli`, `GBufferFormats.h`, `FrameSceneResources`, `UpscalerInputContract`, `ShadowDenoiseContract`, and the Stage 0D display/exposure helpers. No new helper, pass, or provider interface was added by this audit.
- Stage update result: no new stage was required. All P0/P1 findings below are mapped to the existing stages named in their `Required stage` cells, whose prompts already carry the relevant references. Items that remain local deviations or non-production details are explicitly named in Stage 0D and Stage 13 instead of being left as hidden assumptions.

| Priority | File(s) | Symptom | Risk | Required stage | Reference(s) |
| --- | --- | --- | --- | --- | --- |
| P0 resolved | `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`, `Engine/Assets/Shaders/Passes/Deferred/Sky.hlsl`, indirect miss paths | Sky sampling previously entered lighting through a display/tone-mapped helper instead of pure HDR radiance. | Violated the Stage 0 lighting/display boundary and made indirect lighting depend on presentation math. | Stage 0D completed; Stage 1 and Stage 13 keep sky reference captures/import validation. | Filament color/display boundary, PBRT/RTXPT radiance transport. |
| P1 resolved | `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`, `Engine/Assets/Shaders/Passes/Deferred/GBufferUtils.hlsli`, `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`, `Engine/Assets/Shaders/Material/Material.hlsli`, `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`, `Engine/Renderer/Private/Frame/RayTracing/RayTracingHitDataFrameData.cpp`, `Tools/Import/SourceImporters/Private/Gltf/GltfMaterialPropertyMapper.cpp` | Primary GBuffer previously dropped material dielectric `F0`, so deferred direct lighting fell back to `0.04` while ray-hit lighting could use imported/cooked `F0`. | Resolved for current shader paths by storing dielectric F0 in `GBufferMaterial.a` and building F0 through `SurfaceLighting::BuildF0`. Stage 2A now imports scalar glTF IOR into the existing cooked material `F0` field; `KHR_materials_specular` remains a diagnosed unsupported extension because Sparkle does not yet have colored/specular-texture F0. | Stage 2 completed; Stage 2A scalar IOR/F0 import completed; colored/specular-texture workflow remains a documented non-goal until a future material-model stage. | Filament material model, Unreal physically based materials, glTF material extensions. |
| P1 | `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSignals.hlsli`, `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`, `Engine/Renderer/Public/Denoising/ShadowDenoiseContract.h`, `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphDenoiserRegistration.cpp` | Shader shadow helpers define a packed `float4(visibility, hitDistance, confidence, maxDistance)` signal, but registered raw/scratch/history/denoised visibility resources are single-channel `R32_Float`. | Denoiser/provider integration can silently lose hit-distance/confidence data or require pass-local side channels. | Stage 0F, Stage 5, Stage 6. | NRD SIGMA signal contracts, Falcor/RTXPT debug signal ownership. |
| P1 | `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`, glTF importer files under `Tools/Import/SourceImporters/Private/Gltf` | Punctual attenuation uses local range fade and linear cone ramp; the audit did not find `KHR_lights_punctual` import coverage. | Imported lights and authored renderer lights can diverge from glTF/physical light-unit expectations. | Stage 2A and Stage 4. | `KHR_lights_punctual`, Filament/Unreal physical light units. |
| P1 | `Engine/Renderer/Private/Upscaling/UpscalerInputContract.h`, `Engine/Renderer/Private/Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.cpp`, indirect lighting shaders | Provider contract currently covers DLSS SR/NativeAA-style inputs, but not DLRR/indirect reconstruction signals such as noisy indirect radiance, demodulated radiance, lobe id, hit distance, confidence, or variance. | Future indirect reconstruction can become provider-specific and duplicate signal definitions outside the frame graph. | Stage 0F, Stage 11, Stage 11A. | Streamline DLSS Ray Reconstruction, NRD, AMD FidelityFX provider resource patterns. |
| P1 | `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`, `Tools/Cooking/MaterialCooker/Private/TextureCookRequestBuilder.cpp`, sky/environment shaders | The default EXR sky is cooked as a regular linear 2D texture and HDR sky import/orientation/calibration/importance-sampling policy is not yet a single asset contract. | Environment lighting can be visually plausible but physically uncalibrated or inconsistent with later importance sampling. | Stage 1, Stage 2A, Stage 9, Stage 13. | Filament image-based lighting, glTF texture/color-space policy, PBRT/RTXPT environment sampling. |
| P2 | `Engine/Renderer/Private/Frame/Presentation/OutputEncodingSettings.cpp`, presentation shaders, RHI swapchain copy path | Stage 0D documents a local deviation: compute presentation writes encoded UNorm bytes before copy instead of relying on render-target hardware sRGB writes for sRGB swapchains. | Backbuffer encoding policy is explicit and deterministic, but still needs backend-level cleanup before claiming hardware sRGB compliance. | Stage 13 cleanup; deviation remains documented in Stage 0D until a render-target presentation path exists. | Filament display management, Falcor tone mapper/output path. |
| P2 | `Engine/Renderer/Private/Frame/Deferred/GBufferFormats.h`, `Engine/Renderer/Private/Frame/Core/FrameSceneResources.cpp`, frame-graph resource builders | Format, units, history behavior, and provider consumers are discoverable in effect-local code but not consistently referenced from pass/resource declarations. | New passes can repeat or conflict with resource semantics, especially for provider paths. | Stage 0F. | NRD, Streamline, Falcor, AMD FidelityFX resource contracts. |

## Stage 0F: Lock Render Target, Precision, and Signal Surface Contracts

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Create effect-owned signal contracts for PBR-relevant buffers. Each contract must state owner, format, color space, units, valid range, lifetime, history behavior, and consumers without adding a renderer-wide alias namespace. Fix any resource whose declared frame-graph format does not match the shader signal it stores.

Reference lineage:

- NVIDIA NRD resource and signal conventions for visibility, hit distance, normals, roughness, viewZ, motion, and history: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA Streamline DLSS Ray Reconstruction resource tagging and required denoiser inputs: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA Falcor render graph resource ownership patterns: <https://github.com/NVIDIAGameWorks/Falcor>
- AMD FidelityFX SDK resource and temporal input patterns: <https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK>
- Existing Sparkle frame-graph and provider contracts must be the implementation base.

Files:

- `Engine/Renderer/Private/Frame/Deferred/GBufferFormats.h`
- `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphDenoiserRegistration.cpp`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.h`
- `Engine/Renderer/Private/Frame/FrameResources.*`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/LightingComposite.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/OutputEncoding.hlsl`

Signals to document:

- HDR scene color and lighting targets.
- Base color, normal, material, emissive, subsurface, device-Z, motion vector.
- Raw shadow visibility, packed shadow signal, shadow hit distance, denoised visibility.
- Noisy indirect radiance, demodulated indirect radiance, indirect hit distance, lobe id, confidence/variance.
- Albedo, specular/F0, roughness, normals, depth/viewZ, motion vectors, exposure, history reset state.
- Presentation backbuffer and output encoding.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Each signal family has one canonical contract in its owning effect/module docs or comments, and pass/resource registration code references that owner instead of duplicating prose.
- Packed `float4` shadow or indirect signals are stored in matching multi-channel formats or resolved into explicitly named single-channel products before consumers see them.
- Scene lighting and sky remain HDR linear until presentation; presentation output encoding is part of the display signal contract.
- Provider input builders can reject missing or format-incompatible resources at the contract boundary.
- Every denoiser/reconstruction signal has one owner for format, unit, color/geometry space, and lifetime before provider execution.
- Reference compliance: each signal's unit/space/range is mapped to NRD, Streamline DLRR, Falcor, AMD FidelityFX, or the local renderer contract.
- Reuse/DRY: NRD, DLRR, and future denoisers reuse the effect-owned signal contracts instead of duplicate resource descriptions or a needless global format alias layer.

Completion note:

- Reference lineage: `04-PBR-Renderer-Signal-Contract.md` maps signal ownership, format, unit/space, range, lifetime/history, and consumers back to NRD-style denoiser guide resources, Streamline DLRR-style provider tags, Falcor-style render-graph resource ownership, AMD FidelityFX-style temporal/reset resource handling, and Sparkle's local linear-HDR lighting contract.
- Reuse/DRY audit: scanned `FrameRenderFormats`, `GBufferFormats`, `FrameSceneResources`, `LightingRenderTargets`, `FrameGraphDenoiserRegistration`, `ShadowDenoiseContract`, `UpscalerInputContract`, `LightingComposite`, `ToneMapping`, `OutputEncoding`, and the moved `RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli` body before adding the contract. Semantic prose lives in one docs table, while code format constants stay in effect-specific homes instead of a renderer-wide alias namespace.
- Format fix: packed raw shadow signal resources are now `R32G32B32A32_Float` (`ShadowVisibilitySignalRaw` and `ShadowVisibilitySignalScratch`) matching `float4(visibility, hitDistance, confidence, maxDistance)`. Denoised visibility and denoised visibility history are explicitly scalar `R32_Float` products.
- Provider contract: no provider-format shim was added in this stage. `UpscalerInputContract` remains the existing missing-resource/convention gate; real format rejection should be added only when the frame graph exposes actual resource descriptions to provider builders without duplicating constants.
- Source hygiene: no final shader/source comments or Reference lineage banners were added. The prompt's old deferred path for `RayTracedShadowDenoiserInputs.hlsli` is superseded by the Stage 0A module boundary move to `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`.
- Remaining staged work: indirect denoiser/DLRR auxiliary buffers are reserved in the contract but intentionally not allocated until Stage 11/11A.
- Validation commands: rebuilt `ShowcaseEditor` DevelopmentEditor successfully; cooked `ToneMapping` and `OutputEncoding` for `DxilSm66` and `SpirV16` successfully; `LightingComposite` HLSL compiled for `DxilSm66` but package verification failed on the pre-existing reflected binding mismatch between shader cbuffer `PerFrameConstantBufferData` and registration layout alias `PerFrame`, so the full deferred shader-cook gate remains blocked by shader-registration naming cleanup outside the signal-format change.

## Stage 1: Fix Linear HDR Sky Transport

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Split sky sampling into two functions: one that returns linear HDR radiance and one optional display transform for presentation/debug only. Use linear sky radiance for sky pixels, indirect ray misses, mirror reflections, and any future environment sampling. Remove pre-presentation tone mapping from `SampleSkyEnvironment`.

Reference lineage:

- PBRT infinite/environment light treatment: <https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights>
- Google Filament color management and IBL treatment: <https://google.github.io/filament/Filament.md.html>
- NVIDIA RTX Path Tracing SDK sky/environment radiance usage: <https://github.com/NVIDIA-RTX/RTXPT>
- Unreal Engine path tracer reference behavior for environment lighting: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>

Files:

- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/Sky.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl`
- `Engine/Assets/Shaders/Passes/Presentation/OutputEncoding.hlsl`

Expected shader shape:

```hlsl
float3 SampleSkyEnvironmentRadiance(Texture2D environmentTexture, SamplerState environmentSampler, float3 worldDirection)
{
    return max(environmentTexture.SampleLevel(environmentSampler, ComputeSkyEnvironmentUv(worldDirection), 0.0f).rgb, 0.0f.xxx);
}
```

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Sky pass writes linear HDR sky radiance to scene color.
- Presentation still tone maps final scene color exactly once.
- Indirect diffuse/specular miss rays use linear HDR sky radiance.
- Mirror-sky test: a perfect mirror reflection of the sky matches the sky pixel before presentation.
- Bright EXR sky values above 1.0 survive in scene color before presentation.
- Reference compliance: implementation notes map sky sampling and presentation separation to PBRT/Filament/RTXPT behavior.
- Reuse/DRY: all sky consumers call one canonical HDR radiance sampling helper; any display-only sky transform is separate and not copied into indirect passes.

Completion note:

- Reference lineage: sky/environment texture lookup follows PBRT's infinite-area/image light treatment by treating the environment map as emitted radiance sampled by direction. Presentation separation follows Filament color-management discipline, RTXPT-style path transport where misses return environment radiance, and Unreal path-tracer-style reference behavior where environment lighting remains part of scene transport before display conversion.
- Reuse/DRY audit: scanned `SkyEnvironment.hlsli`, `Sky.hlsl`, `IndirectDiffuse.hlsl`, `IndirectSpecular.hlsl`, `ToneMapping.hlsl`, and `OutputEncoding.hlsl` before editing. The single sky transport helper is now `SampleSkyEnvironmentRadiance`; sky pixels, diffuse miss rays, and specular/mirror miss rays all call it. No display-only sky helper was added because no display-only sky consumer exists; presentation already owns tone mapping and output encoding.
- Implementation: `SampleSkyEnvironmentRadiance` samples the environment map with `ComputeSkyEnvironmentUv` and clamps only negative samples to zero, preserving HDR values above `1.0` in scene color and indirect miss radiance.
- Validation note: a perfect mirror miss and a sky pixel now use the same HDR radiance helper before presentation, so they match for the same world direction except for the existing view/reflection direction mapping and later material throughput.

## Stage 2: Unify Primary and Ray-Hit Material Data

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Make deferred primary shading and ray-hit shading use the same dielectric F0 value and material roughness convention. Add F0/reflectance to the GBuffer or derive it from a documented material constant that is available in direct lighting. Avoid using fixed `0.04` for primary surfaces when material F0 exists.

Reference lineage:

- glTF 2.0 metallic-roughness material model: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials>
- Google Filament material/reflectance model: <https://google.github.io/filament/Filament.md.html>
- Unreal Engine physically based materials: <https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine>
- Epic "Real Shading in Unreal Engine 4": <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>

Files:

- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferUtils.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/Renderer/Private/Frame/Deferred/GBufferFormats.h`

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Direct primary lighting and ray-hit direct lighting use the same F0 for the same material.
- A dielectric material with non-default reflectance visibly affects primary and reflected lighting consistently.
- Material roughness is preserved as an inclusive `[0, 1]` input contract and is not used as a hidden F0 or light-scale compensation path.
- GBuffer format changes, if any, are reflected in visualization modes and shader binding metadata.
- Reference compliance: material/F0 mapping is explicitly traced to glTF, Filament, or Unreal terminology.
- Reuse/DRY: primary and ray-hit material decoding share canonical helpers or documented common data contracts instead of separately deriving material constants.

Completion note:

- Reference lineage: the material contract follows the glTF metallic-roughness split of base color, metallic, and roughness; Filament/Unreal-style dielectric reflectance is carried by Sparkle's existing scalar material `f0` and blended with base color for metallic surfaces. The local deviation is that Stage 2 does not add a Filament-style reflectance UI/remap or new glTF specular-extension import; Stage 2A owns that import/material authoring work.
- Reuse/DRY audit: scanned `Material.hlsli`, `GBufferPS.hlsl`, `GBufferUtils.hlsli`, `DirectLighting.hlsl`, `SurfaceLighting.hlsli`, `RayTracingMaterialHit.hlsli`, `RayTracingHitLighting.hlsli`, `PathSurface.hlsli`, indirect diffuse/specular entrypoints, `GBufferFormats.h`, and visualization shader bindings before editing. The change reuses the existing `GBufferMaterial` target and `SurfaceLighting` home; no new format registry, shader module, pass, or GBuffer format was added.
- Implementation: `GBufferMaterial.a` now stores dielectric F0, direct lighting passes `gBuffer.DielectricF0`, primary indirect path surfaces use the same value, ray-hit reconstruction keeps material `F0` as actual F0, and ray-hit direct lighting shares `SurfaceLighting::BuildF0`.
- Roughness remains the authored/imported inclusive `[0, 1]` material value. Stage 2 does not clamp it into an F0 or light-scale compensation path; Stage 2C now defines the full-range roughness and singularity policy for the current shader paths.
- No `GBufferFormats.h` change was required, so shader binding metadata and visualization resource declarations remain valid. Base-color alpha still carries blend alpha; ray-hit alpha mode remains in ray-tracing hit data instead of the material GBuffer.
- Validation commands: rebuilt `ShowcaseEditor` and `ShaderCompiler`; cooked `GBuffer`, `DirectLightingNoRayQuery`, `DirectLighting`, `DirectLightingVulkanAddress`, `IndirectDiffuse`, `IndirectSpecular`, and `VisualizeBuffers` for `DxilSm66` and `SpirV16`. `LightingComposite` HLSL compiled for both targets, but full package cook remains blocked by the pre-existing reflected binding mismatch between shader cbuffer `PerFrameConstantBufferData` and registration layout alias `PerFrame`.

## Stage 2C: Full Roughness Range and Reference Roughness Policy

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Make the renderer support perceptual roughness from `0.0` through `1.0` with a reference-backed policy that separates material data, BRDF evaluation, and sampling behavior. Direct analytic lighting must call the same canonical BRDF evaluator for all roughness values. Do not add pass-local direct-light branches that classify roughness and bypass specular evaluation. Perfect-mirror transport decisions belong in BSDF/path-sampling code, where delta lobes are sampled as exact directions, not in primary or ray-hit direct-light pass code. Extremely rough reflections must remain valid at `roughness = 1.0`.

Reference lineage:

- PBRT specular reflection/delta BSDF and rough dielectric/conductor separation: <https://pbr-book.org/4ed/Reflection_Models>
- Google Filament material roughness and GGX conventions: <https://google.github.io/filament/Filament.md.html>
- Epic "Real Shading in Unreal Engine 4" roughness/GGX conventions: <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>
- NVIDIA RTX Path Tracing SDK material and path sampling structure: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor BSDF/path-tracing modules: <https://github.com/NVIDIAGameWorks/Falcor>

Files:

- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/Debug/ViewModes.hlsli`

Required design:

- Material roughness must be preserved as authored/imported data and must remain visible to debug and denoiser outputs.
- Direct primary lighting and ray-hit direct lighting must share one direct-surface evaluator, and that evaluator must call `BRDF::Direct::Evaluate` for all roughness values.
- Numerical guards for singular GGX cases must live directly inside the owning BRDF distribution/geometry/PDF expressions and be backed by references. Do not add alias constants unless they remove real duplication. They must not silently mutate stored material roughness.
- Indirect specular/path sampling may branch on lobe type because sampling a delta lobe is a different estimator than sampling a finite rough lobe. That branch must live in shared BSDF/path-sampling code, not in direct-light pass logic.
- Indirect specular must sample exact reflection for true mirror transport and GGX/VNDF or the selected rough-specular sampler for finite rough-specular transport.
- Indirect diffuse must pass material roughness through diffuse models unchanged; diffuse roughness may be zero for smooth dielectrics.
- Ray-hit direct lighting and primary direct lighting must use the same BRDF, roughness value, F0 policy, and light-unit policy.
- Denoiser/debug roughness outputs must store material roughness, not internally floored roughness.
- Numerical epsilons remain allowed only for denominators, PDFs, ray origin/ray range safety, and reference-backed BRDF singularity guards. Prefer inline `EPSILON` or the local numeric threshold at the use site over one-off aliases, and do not alter material roughness or direct-light control flow.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- `rg` finds no direct-light lobe bypass helpers or pass-local mirror/specular branches in `SurfaceLighting.hlsli` or `RayTracingHitLighting.hlsli`.
- `rg` finds no caller-side material roughness floors such as `max(roughness, 0.04)` or `max(surface.Roughness, 0.04)` in direct-lighting paths.
- Any remaining `max`, `clamp`, or `saturate` involving roughness or alpha is classified as input range enforcement, debug visualization, denominator/PDF safety, or reference-backed BRDF singularity protection. Implementation notes must name why it is not a hidden material roughness mutation.
- Roughness `0.0`, `0.02`, `0.1`, `0.5`, and `1.0` render without NaN/Inf in direct, indirect diffuse, indirect specular, and ray-hit direct paths.
- Mirror-sky validation confirms a `roughness = 0.0` mirror reflects the HDR sky consistently before presentation.
- Direct and ray-hit direct lighting agree for the same material roughness, F0, normal, and light input.
- Reference compliance: implementation notes map direct BRDF evaluation, GGX rough-specular, mirror/delta path sampling, numerical singularity guards, and any punctual-light mirror behavior to PBRT/Filament/Unreal/RTXPT/Falcor references or document a non-reference approximation.
- Reuse/DRY: direct lighting has one canonical BRDF evaluation path; lobe sampling decisions are centralized in shared BSDF/path-sampling modules and are not forked into pass-local direct-light helpers.

Completion notes:

- Reference compliance: primary direct and ray-hit direct lighting keep the Filament/Unreal-style metallic-roughness GGX path by calling `BRDF::Direct::Evaluate` through `Lighting/SurfaceLighting.hlsli` for every material roughness value. PBRT's separation between delta specular transport and finite rough BSDF evaluation is followed by moving mirror-vs-GGX sampling into `BRDF/SpecularSampling.hlsli`, not into the direct-light passes. The RTXPT/Falcor module pattern is followed only at the local shader-module level: pass entrypoints delegate lobe sampling to a shared BRDF-owned include while full unified BSDF path sampling remains Stage 7 work.
- Local deviations: finite rough-specular sampling still uses the existing GGX NDF half-vector sampler rather than VNDF/MIS. Analytic punctual direct lighting does not synthesize a delta mirror response for `roughness = 0.0`; exact mirror transport is handled by the indirect/path sampling code, and the punctual-light finite-light/area-light policy remains Stage 4/4A work.
- Reuse/DRY audit: scanned `BRDF/*.hlsli`, `Material.hlsli`, `Lighting/SurfaceLighting.hlsli`, `RayTracing/RayTracingHitLighting.hlsli`, `RayTracing/RayTracingMaterialHit.hlsli`, `IndirectDiffuse.hlsl`, `IndirectSpecular.hlsl`, `IndirectSpecularDebug.hlsli`, and `Debug/ViewModes.hlsli` before editing. The change reuses the existing BRDF and lighting module homes; the only new include is `BRDF/SpecularSampling.hlsli`, which replaces pass-local specular lobe sampling rather than adding a renderer/pass layer.
- Implementation: material roughness remains the authored/imported inclusive `[0, 1]` value through GBuffer decode, ray-hit material reconstruction, debug roughness output, direct lighting, indirect diffuse throughput, and indirect specular sampling. `IndirectSpecular.hlsl` now asks `BRDF::SpecularSampling::SampleReflectionLobe` for either an exact mirror sample at `roughness = 0.0` or a finite GGX reflection sample; invalid finite samples return zero throughput instead of silently falling back to mirror transport.
- Simplification audit: removed the pass-local forwarding wrapper around specular lobe sampling, removed single-use mirror/invalid sample constructor wrappers, removed the single-use indirect-specular sky miss wrapper, removed shadow sampling aliases that only forwarded into `CommonSampling`, removed material one-liners that only wrapped `saturate`/normal unpacking/texture sampling, removed unused BRDF transport wrappers, and collapsed ray-hit material texture forwarding helpers into one adapter that owns packed-index resolution plus table sampling. Kept helpers that own actual sampling math, throughput evaluation, tracing policy, material texture fallback policy, packed signal layout, BRDF model dispatch, or debug routing.
- Guard classification: inline `EPSILON` uses in BRDF distribution, geometry, specular, diffuse, and subsurface code are denominator guards. Inline `1.0e-4f` uses in specular sampling and indirect throughput are PDF/sample guards local to those expressions. `saturate`/range clamps in material decode and GBuffer/ray-hit unpack are input range enforcement; `ViewModes` clamps are debug visualization; ray-origin and ray-range constants remain ray safety. None of these guards mutates stored material roughness or changes direct-light control flow.
- Validation commands: rebuilt `ShaderCompiler` and `ShowcaseEditor`; cooked `GBuffer`, `DirectLightingNoRayQuery`, `DirectLighting`, `DirectLightingVulkanAddress`, `IndirectDiffuse`, and `IndirectSpecular` for `DxilSm66` and `SpirV16`. Code validation confirms the mirror-sky path uses exact reflection through `BRDF::SpecularSampling::SampleReflectionLobe` and linear HDR sky miss radiance through `SampleSkyEnvironmentRadiance`; image capture validation remains Stage 13 validation asset work.

## Stage 2A: Lock Asset Color-Space, Material Extension, and Import Unit Rules

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Turn asset import and texture cooking into an enforceable PBR contract. The shader can only be physically meaningful if imported base color, data maps, HDR sky maps, packed metallic-roughness channels, material reflectance, emissive strength, and light intensity units arrive in the spaces expected by the BRDF and light equations.

Reference lineage:

- glTF 2.0 material texture and metallic-roughness definitions: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#materials>
- glTF `KHR_lights_punctual` units and cone semantics: <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual>
- Google Filament material and color-management documentation: <https://google.github.io/filament/Filament.md.html>
- Unreal Engine physically based materials and physical light units documentation.
- Existing Sparkle import/cook code must be reused and tested before adding new import paths.

Files:

- `Engine/Assets/Private/Texture/*`
- `Tools/Import/SourceImporters/Private/Gltf/GltfMaterialTextureMapper.cpp`
- `Tools/Import/SourceImporters/Private/Gltf/GltfMaterialPropertyMapper.cpp`
- `Tools/Import/SourceImporters/Private/Gltf/GltfLightImporter.cpp`
- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferPS.hlsl`
- `Engine/Assets/Shaders/Lighting/SkyEnvironment.hlsli`
- `Docs/Rendering/PBR/*`

Required policy:

- Base color, emissive color, and subsurface color are decoded from sRGB/color space into linear shading values.
- Normal, roughness, metallic, ambient occlusion, subsurface strength, F0/reflectance, packed material channels, and scalar masks are imported as data.
- HDR sky/environment textures stay linear HDR and are never display tone mapped before lighting.
- glTF metallic-roughness packed channel mapping is tested: roughness in green, metallic in blue.
- `KHR_materials_specular`, `KHR_materials_ior`, and `KHR_materials_emissive_strength` are implemented or fall back through a documented material policy.
- Specular-workflow texture sets such as Bistro `*_Specular` maps are converted into the supported material model, routed through a declared non-reference workflow, or ignored through a documented fallback policy.
- `KHR_lights_punctual` intensities are preserved or converted once into documented engine units.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Asset-cook policy enforces color/data/HDR decode and compression choices.
- glTF metallic-roughness channels reach shaders unchanged except for intended normalization.
- Specular-workflow assets are converted, routed, or ignored according to the documented policy.
- A non-default dielectric F0/reflectance material produces matching primary deferred and ray-hit material values after Stage 2.
- A glTF light calibration scene proves point/spot attenuation ratios at known distances after Stage 4.
- HDR sky import and sampling preserve values above 1.0 until presentation after Stage 1.
- Reference compliance: each color-space, packed-channel, extension, and light-unit decision cites glTF/Filament/Unreal in the implementation prompt or source comments.
- Reuse/DRY: importers, cook policies, and shader material decode share one material/texture-usage vocabulary; no separate tool-side and shader-side assumptions are allowed without a named conversion.

Stage 2A implementation notes:

- Reference lineage: color/data texture policy follows glTF's material texture roles and Filament/Unreal's color-vs-data split by keeping base color, emissive, and subsurface color in sRGB cook policy while normal, roughness, metallic, AO, masks, F0, and HDR color remain linear/data. Metallic-roughness packed-channel handling follows glTF's roughness-in-G and metallic-in-B convention. glTF `KHR_materials_emissive_strength` is applied as a multiplier to the imported emissive factor. glTF `KHR_materials_ior` maps to Sparkle's existing scalar dielectric `F0` with the normal-incidence reflectance equation also used by Filament/Unreal-style dielectric material models. `KHR_lights_punctual` values are still imported without hidden tone-mapping compensation; Stage 4 owns attenuation/unit calibration.
- Local deviations: `KHR_materials_specular` is rejected by material policy instead of partially converted because Sparkle currently stores scalar dielectric `F0` and has no colored/specular-texture F0 material channel. Legacy `KHR_materials_pbrSpecularGlossiness` remains an approximate diffuse/roughness import through a documented unsupported-feature fallback. FBX/Assimp specular-workflow textures are no longer routed into metallic or AO; they fall back silently to Sparkle's current material defaults until a real conversion policy exists.
- Reuse/DRY audit: scanned `TextureCookRequestBuilder`, texture cook format/compression policy, glTF material texture/property/feature importers, FBX material importer, light importer, material shader decode, GBuffer write/decode, and sky sampling before editing. The stage reuses existing `TextureGroup`, `ImportedMaterial`, cooked material `F0`, texture channel masks, and import diagnostic logs; it adds no renderer-wide registry, wrapper namespace, or duplicate texture-usage vocabulary.
- Simplification audit: removed the hidden FBX specular-to-metallic/AO fallback without adding replacement reporting or helper indirection. No helper was added for IOR or emissive strength; the policy lives directly in the existing glTF property mapper.
- Validation commands: built `SourceImporters`, `MaterialCooker`, `AssetCooker`, and `TextureCooker` for `DevelopmentEditor`; ran `AssetCooker collect-texture-requests Projects\Showcase\Assets\Meshes\DamagedHelmet\DamagedHelmet.gltf build\stage2a\damagedhelmet-texture-requests.txt`; ran `TextureCooker inspect-request-file` and `cook-request-file` on that request list. The inspection verified diffuse/emissive as sRGB-linearized, normal/AO/roughness/metallic as linear/data, AO in red, roughness in green, and metallic in blue; the cook completed 6/6 textures.
- Remaining validation: there is no checked-in Bistro scene file or extension fixture for `KHR_materials_ior`, `KHR_materials_emissive_strength`, or `KHR_materials_specular`, so deterministic image/material-value validation remains Stage 13 validation-pack work.

## Stage 2B: Lock Geometry, Normal, Depth, Motion, and Temporal Conventions

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Define one convention for the geometry and temporal signals used by shading, ray tracing, denoisers, DLRR, and reference paths. This stage should happen before expanding denoiser resources, because wrong normal/depth/motion conventions produce plausible but non-physical reconstructed lighting.

Reference lineage:

- NVIDIA NRD integration documentation for normal encoding, viewZ/depth, motion vectors, jitter, and history reset conventions: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA Streamline DLSS Ray Reconstruction resource tagging and normal/roughness/depth/motion expectations: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA RTXPT and Falcor ray-origin, hit-surface, and path-state conventions: <https://github.com/NVIDIA-RTX/RTXPT>, <https://github.com/NVIDIAGameWorks/Falcor>
- PBRT shading normal and ray-offset correctness discussion as offline reference: <https://pbr-book.org/4ed/Shapes/Managing_Rounding_Error>
- Existing Sparkle GBuffer, TLAS, material-hit, and provider contract code.

Files:

- `Engine/Assets/Shaders/Geometry/*.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingTraceQuery.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/GBuffer*.hlsl*`
- `Engine/Assets/Shaders/Passes/Deferred/Velocity.hlsl`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.*`
- `Engine/Renderer/Private/Frame/Deferred/GBufferFormats.h`
- `Engine/RHI/Public/Resources/RenderView*.h`

Conventions to lock:

- World-space vs view-space normal ownership for GBuffer, ray-hit shading, denoiser inputs, and reference paths.
- Geometric normal, shading normal, face orientation, two-sided materials, normal-map handedness, and tangent-frame construction.
- Ray-origin offset and self-intersection policy for primary, shadow, indirect, and reference rays.
- Device depth vs linear viewZ, reversed-depth convention, and provider conversion location.
- Motion-vector units, sign, jitter inclusion/exclusion, current-to-previous matrix policy, camera cut, resize, and history reset behavior.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- A convention table names the owner and consumer for each geometry/temporal signal.
- Primary GBuffer shading and ray-hit shading use the same normal/orientation convention.
- Motion vectors use the provider contract direction, units, and jitter policy.
- Denoiser and DLRR provider builders receive normal/depth/motion signals in one documented space or perform one documented conversion.
- Alpha-tested and two-sided materials use the same face/orientation rules in primary, shadow, indirect, and reference paths.
- Reference compliance: every convention maps to NRD, Streamline DLRR, RTXPT/Falcor, PBRT, or a documented local engine decision.
- Reuse/DRY: normal decode, ray offset, depth conversion, motion-vector conversion, and history-reset logic live in shared helpers or provider-contract builders, not pass-local copies.

Stage 2B implementation notes:

- Reference lineage: geometry/provider conventions are locked in `04-PBR-Renderer-Signal-Contract.md` against NRD/Streamline expectations for normal/depth/motion/history metadata, RTXPT/Falcor-style hit-surface records, and PBRT's explicit ray-offset/self-intersection policy. Local deviations are called out for Sparkle's current device-depth storage, local ray-bias constants, and direct-shadow alpha/two-sided parity remaining Stage 5 work.
- Reuse/DRY audit: scanned `Geometry/Basis.hlsli`, `Geometry/Transforms.hlsli`, `Material.hlsli`, GBuffer vertex/pixel/util shaders, ray-hit material reconstruction, ray-query tracing, shadow tracing, `GBufferFormats`, temporal constants/state, `UpscalerInputContract`, the DLSS Streamline bridge, and existing PBR signal docs before editing. Existing homes are reused: `Geometry/Basis.hlsli` owns tangent-frame construction, `MotionVector.hlsli` owns pixel motion-vector math, `UpscalerInputContract` owns provider depth/motion conventions, and `TemporalFrameState` owns history-valid state.
- Implementation: primary GBuffer tangent basis construction now uses `OrthonormalizeTangent` and `ComputeBitangentFromSign`, matching ray-hit reconstruction. Primary material normal mapping now calls `TransformTangentNormalToWorld`, the same canonical helper used by ray-hit shading. Stale geometry helpers that duplicated basis transforms, local/world position wrapper chains, and unused rotation math were removed from `Geometry/Transforms.hlsli`.
- Simplification audit: no new provider enum, registry, pass, or wrapper was added. The only code change deletes duplicate/unused helpers and routes primary shading through existing concept-owned helpers.
- Validation commands: cooked `GBuffer`, `DirectLightingNoRayQuery`, `DirectLighting`, `DirectLightingVulkanAddress`, `IndirectDiffuse`, and `IndirectSpecular` for `DxilSm66` and `SpirV16`. Deterministic image tests for primary-vs-ray-hit normal parity, motion-vector movement, and alpha-tested direct-shadow parity remain Stage 13/Stage 5 validation asset work.

## Stage 3: Normalize BRDF Energy Behavior

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Normalize the production BRDF around one explicit energy policy instead of adding diagnostic paths. Keep the current GGX/Smith/Schlick metallic-roughness model only if the code makes its energy terms clear, removes unused model switchboards, and keeps direct, indirect, and ray-hit lighting on the same BRDF functions. Decide whether the strict reference diffuse term is Lambert or production Burley, and make subsurface a documented non-reference lobe until it is energy-compensated.

Reference lineage:

- Epic "Real Shading in Unreal Engine 4" GGX/Smith/Schlick conventions: <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>
- Google Filament BRDF and furnace-test discussion: <https://google.github.io/filament/Filament.md.html>
- PBRT BSDF energy/reference behavior: <https://pbr-book.org/4ed/Reflection_Models>
- Walt Disney BRDF notes for Burley diffuse, if Burley remains enabled in production mode.

Files:

- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- BRDF contract notes under `Docs/Rendering/PBR`.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Unused BRDF model dispatch, dead reference toggles, and pass-local BRDF math are removed unless a real production path still uses them.
- Diffuse white, mixed dielectric, and metallic materials have an explicit lobe energy budget; `diffuse + specular + subsurface` cannot silently exceed the chosen opaque-material policy.
- No material path returns NaN or infinity at grazing NoV/NoL.
- The meaning of `Geometry::EvaluateDirect` is documented as visibility `V` or raw geometry `G`.
- Subsurface contribution is either excluded from the reference/PBR energy path or energy compensated against diffuse.
- Reference compliance: each kept BRDF helper names the convention it follows; deleted alternatives are not left behind as confusing half-supported paths.
- Reuse/DRY: direct, indirect, and ray-hit lighting call the same BRDF helpers; no pass-local duplicate BRDF math remains.

## Stage 3A: Centralize Shading Model and Lobe Energy Policy

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Centralize the supported shading models and lobe weights in the existing material/BRDF homes. Do not introduce a registry or new naming layer unless the current material data cannot express the policy. Direct lighting, indirect sampling, ray-hit lighting, path tracing, and provider-facing demodulation must agree on which lobes exist, how they are weighted, which lobes are sampled, and how energy is conserved between them.

Reference lineage:

- Google Filament material model, reflectance, and energy-conservation discussion: <https://google.github.io/filament/Filament.md.html>
- Epic "Real Shading in Unreal Engine 4" metallic/specular/diffuse conventions: <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>
- Unreal Engine physically based material parameter documentation.
- PBRT BSDF/lobe reference model: <https://pbr-book.org/4ed/Reflection_Models>
- NVIDIA RTXPT/Falcor path tracing BSDF organization for lobe selection and material/path modularity.

Files:

- `Engine/Assets/Shaders/Material/Material.hlsli`
- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- Future shared path-sampling files from Stage 7.
- Material import/default settings files.

Required policy:

- Declare supported lobes for the reference path: diffuse, dielectric specular, conductor specular, emissive, and optional subsurface.
- Define lobe weights and PDFs from material data once, then reuse them in direct evaluation, BSDF sampling, and provider-facing demodulation.
- Ensure `diffuse + specular + subsurface/transmission/future lobes` does not exceed the intended energy budget for opaque reference materials.
- Mark subsurface as disabled in reference mode until it is energy compensated against diffuse or implemented as a documented BSSRDF/approximation.
- Keep artist-facing non-reference modes possible, but make them explicit deviations from the reference path.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- The lobe policy lives in the smallest existing material/BRDF boundary that can own it; no new registry is added unless it replaces scattered logic.
- Diffuse, dielectric specular, metallic, rough metal, and subsurface-enabled materials all resolve through the same lobe energy policy.
- Direct lighting and indirect BSDF sampling use the same lobe definitions and roughness/F0 policy.
- Provider demodulation uses the same diffuse/specular material factors as the BSDF/lobe policy.
- Reference compliance: lobe weights and conservation rules cite Filament/Epic/PBRT/RTXPT/Falcor and document local deviations.
- Reuse/DRY: no pass computes its own lobe split, diffuse albedo, specular albedo, or subsurface energy outside the shared material/BRDF policy.

## Stage 4: Fix Punctual Light Units and Falloff

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Define engine units for directional, point, and spot lights. Align glTF-imported point/spot intensity with candela and directional intensity with lux or a documented calibrated engine unit. Replace current range and cone attenuation with a glTF/Filament-compatible punctual falloff policy, or document a deliberate engine policy and convert imports into that policy.
The same unit workflow must be visible in editor controls, level serialization, cooked light records, runtime scene data, render-view constant buffers, and shader consumers; it is not allowed to exist only as importer-side knowledge.

Reference lineage:

- glTF `KHR_lights_punctual` light units and cone model: <https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual>
- Google Filament punctual light units/falloff: <https://google.github.io/filament/Filament.md.html>
- Unreal Engine physical light units documentation and behavior: <https://dev.epicgames.com/documentation/en-us/unreal-engine/physical-lighting-units-in-unreal-engine>
- Epic "Real Shading in Unreal Engine 4" direct-lighting conventions: <https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf>

Files:

- `Tools/Import/SourceImporters/Private/Gltf/GltfLightImporter.cpp`
- `Engine/Editor/Private/Panels/SceneLightInspector.cpp`
- `Engine/GameFramework/Private/Level/Parsing/LightingSectionParser.cpp`
- `Engine/GameFramework/Public/Assets/Cooked/CookedSceneLightRecord.h`
- `Engine/GameFramework/Private/Scene/Lighting/Snapshots/SceneLightingSnapshotBuilder.cpp`
- `Engine/Renderer/Public/SceneData/*.h`
- `Engine/Assets/Shaders/Resources/LightConstantBufferData.hlsli`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/RHI/Public/Resources/RenderViewLightingData.h`

Recommended shader policy:

```hlsl
float DistanceFalloff(float distanceToLight, float range)
{
    float d2 = max(distanceToLight * distanceToLight, 1.0e-4f);
    if (range <= 0.0f)
    {
        return rcp(d2);
    }

    float x = distanceToLight / max(range, 1.0e-4f);
    float smooth = saturate(1.0f - x * x * x * x);
    return smooth * smooth * rcp(d2);
}

float SpotFalloff(float cosTheta, float innerCos, float outerCos)
{
    float t = saturate((cosTheta - outerCos) / max(innerCos - outerCos, 1.0e-4f));
    return t * t;
}
```

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Point and spot light falloff matches the selected unit policy.
- glTF `KHR_lights_punctual` imports do not need arbitrary intensity compensation.
- Editor UI, serialized levels, cooked records, runtime scene data, render-view buffers, and shaders expose the same unit contract: directional lights in lux, point and spot lights in candela.
- Primary direct and secondary hit direct use the same falloff and cone functions.
- The unit policy is explicit enough that expected luminance at known distances can be derived without per-asset compensation.
- Reference compliance: unit conversion and falloff formulas cite glTF/Filament/Unreal and document any deviation.
- Reuse/DRY: direct primary lighting, ray-hit lighting, and import conversion use one canonical punctual-light policy.

Completion note:

- Reference lineage: Sparkle now follows the glTF `KHR_lights_punctual` and Filament punctual-light convention for units: directional intensity is illuminance in lux, point and spot intensity are luminous intensity in candela, point/spot distance falloff is inverse-square, and range is an optional smooth cutoff instead of an intensity unit conversion. Spot cone attenuation follows the glTF smooth cone rule with a squared interpolation term. Unreal/Epic remain the direct-lighting material/BRDF reference for consuming the resulting incident light in the shared surface evaluator.
- Local deviation: finite source radius still affects stochastic shadow sampling only; it does not turn point/spot lights into physically integrated area lights. Stage 4A owns that finite-light policy.
- Reuse/DRY audit: scanned `GltfLightImporter`, editor light controls, level light parsing/serialization, cooked scene light records/builders, scene-asset light translation, `SceneLightingSnapshotBuilder`, render-light upload, `RenderViewLightingData`, `LightConstantBufferData`, `PunctualLights`, `SurfaceLighting`, `RayTracingHitLighting`, `DirectLighting`, and shadow trace consumers before editing. The implementation keeps the canonical falloff and cone math in `Lighting/PunctualLights.hlsli`; primary direct lighting and ray-hit direct lighting already call those helpers, so no wrapper or new policy namespace was added.
- Implementation: `PunctualLights::ComputeDistanceAttenuation` now uses `1 / max(distance^2, 1.0e-4)` with the `smooth^2` `1 - x^4` range cutoff, and `ComputeSpotConeAttenuation` now squares the cone ramp. glTF/imported and authored light intensity values pass through without arbitrary compensation. The editor labels directional intensity as lux and point/spot intensity as candela; serialized levels now write `IntensityLux` or `IntensityCandela` while still accepting older `Intensity` fields. Cooked/runtime/render-view/shader light structs carry the same unit contract. Spot outer cone defaults now match the glTF default `pi/4` where local light records are initialized.
- Build checks: cooked `DirectLightingNoRayQuery`, `DirectLighting`, `DirectLightingVulkanAddress`, `IndirectDiffuse`, and `IndirectSpecular` for `DxilSm66` and `SpirV16`; built `ShowcaseEditor`, `SourceImporters`, `SceneCooker`, and `ShaderCompiler` for `DevelopmentEditor`.

## Stage 4A: Define Finite Light Source and Soft-Shadow Approximation Policy

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Separate the current source-radius soft-shadow approximation from physically integrated finite-area light radiance. A light radius used only to jitter shadow rays must not silently change the radiometric meaning of a point/spot light. If finite lights are added, sample the light source and evaluate PDFs/geometry terms explicitly.

Reference lineage:

- PBRT area-light sampling and geometry terms: <https://pbr-book.org/4ed/Light_Sources/Area_Lights>
- PBRT direct-light sampling/MIS at path vertices: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- NVIDIA RTXPT/Falcor direct-light and mesh/area-light sampling patterns.
- Unreal Engine and Filament punctual-light documentation as the reference for non-area punctual behavior.
- Existing Sparkle ray-traced shadow sampling code must be audited before adding area-light behavior.

Files:

- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Lighting/SurfaceLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/RHI/Public/Resources/RenderViewLightingData.h`
- Light import and scene snapshot code.

Required policy:

- Punctual mode: source radius may affect stochastic shadow penumbra only and does not change emitted intensity or direct BRDF radiance.
- Area-light mode, if introduced: sample a position/direction on the light, evaluate geometry term, emission, visibility, and PDF, then use MIS when combined with BSDF sampling.
- The light data model must explicitly say whether a light is evaluated as punctual-with-soft-shadow or physically finite.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Changing source radius in punctual mode changes penumbra shape but not unoccluded irradiance beyond documented approximation tolerance.
- Any physically finite-light mode has explicit light sampling PDFs and does not reuse point-light attenuation as if it were an area-light integral.
- Primary direct lighting, secondary direct lighting, and path tracing use the same light classification.
- Unoccluded intensity is invariant under source-radius changes in punctual mode except for documented approximation limits.
- Reference compliance: soft-shadow approximation and any finite-light integration cite PBRT/RTXPT/Falcor/Unreal/Filament and document deviations.
- Reuse/DRY: light classification, radius policy, and sampling are stored once and reused by direct, shadow, and indirect code.

## Stage 5: Make Direct Shadows Physically Usable

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Route direct shadow rays through the same alpha-tested candidate handling used by indirect ray queries, or add an equivalent shadow-specific alpha-test resolver. Then split raw stochastic visibility from direct lighting accumulation so NRD SIGMA can consume it later. Treat binary visibility and denoiser hit distance as separate signals: first-hit ray-query shortcuts are acceptable for immediate hard-shadow visibility, but NRD-style occluder-distance input needs a reference-compatible tracing policy and resource format.

Reference lineage:

- NVIDIA RTX Path Tracing SDK alpha-tested ray tracing and visibility handling: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA NRD SIGMA shadow denoiser signal expectations: <https://github.com/NVIDIA-RTX/NRD>
- Microsoft DirectX Raytracing samples for shadow-ray separation patterns: <https://github.com/microsoft/DirectX-Graphics-Samples>
- Unreal Engine ray tracing/path-tracing material visibility behavior: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>

Files:

- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingTraceQuery.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowDenoiserInputs.hlsli`
- `Engine/Renderer/Private/FrameGraph/Resources/FrameGraphDenoiserRegistration.cpp`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/*`

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Alpha-tested foliage/card geometry casts cutout direct shadows.
- Hard shadows and one-sample soft shadows still work on descriptor TLAS and Vulkan address paths.
- Raw visibility and hit-distance targets can be written independently from lighting contribution.
- Shadow hit distance for denoising is generated with a trace mode compatible with the selected NRD SIGMA input rules, not merely inferred from a binary first-hit visibility shortcut.
- Packed raw shadow signals use resource formats that match `RayTracedShadowDenoiserInputs.hlsli`, or an explicit unpack/resolve pass produces the single-channel products consumed downstream.
- Existing direct-light visual output remains unchanged when denoiser mode is off.
- Reference compliance: raw visibility, hit distance, alpha-test policy, and denoiser signal semantics are checked against RTXPT/NRD/DXR samples.
- Reuse/DRY: direct shadows and indirect ray queries share alpha-test and ray-query candidate handling, or a documented shadow-specific adapter calls the same material alpha logic.

## Stage 6: Add Shadow Denoiser Integration Point

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Implement the renderer-side pass boundary for NRD SIGMA without forcing all platforms to have NRD. The pass should consume raw visibility, normal, depth, motion vectors, and history, then output denoised visibility. Direct lighting should be able to consume either raw or denoised visibility through an explicit mode.

Reference lineage:

- NVIDIA NRD repository and SIGMA integration documentation: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA NRD sample/integration code for persistent history resources and signal layout: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA RTXDI integration guide for explicit visibility/reservoir/provider boundaries where applicable: <https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md>
- Existing Sparkle provider/resource contract must be reused instead of introducing a separate denoiser ownership model.

Files:

- Provider-neutral denoiser interface under renderer provider architecture.
- `FrameGraphDenoiserRegistration`
- Shadow settings and pass data.
- Direct lighting pass resource declarations.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- `r.RayTracedShadows.Denoiser=0` uses raw visibility.
- `r.RayTracedShadows.Denoiser=1` requests SIGMA only when the provider boundary can satisfy the required resources; otherwise it uses the existing raw-visibility path without adding noisy local fallback code.
- History resources are persistent and reset on camera cut, resize, and feature toggles.
- Reference compliance: resource names, signal ranges, history reset behavior, and provider fallback behavior map to NRD SIGMA guidance.
- Reuse/DRY: the denoiser path reuses provider-neutral frame graph/product infrastructure and does not fork shadow visibility resource ownership.

## Stage 7: Replace Split Bounce Logic With Unified BSDF Path Sampling

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Create a shared ray-traced surface path sampling module that can sample diffuse or specular lobes from one material using a single lobe-selection PDF. Use it for both indirect diffuse and indirect specular passes. Preserve current output split by assigning the path contribution to the buffer for the primary sampled lobe.

Reference lineage:

- PBRT path tracer throughput and BSDF sampling: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- NVIDIA RTX Path Tracing SDK path state and BSDF sampling structure: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor path tracing/render-pass structure: <https://github.com/NVIDIAGameWorks/Falcor>
- Google Filament BRDF conventions for real-time metallic-roughness inputs: <https://google.github.io/filament/Filament.md.html>

Files:

- New shared HLSL include under `Engine/Assets/Shaders/RayTracing` or `Engine/Assets/Shaders/Lighting`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/BRDF/*.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingPathSample.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSurface.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathSampling.hlsli`
- `Engine/Assets/Shaders/RayTracing/PathLighting.hlsli`

Target algorithm:

```text
surface = primary
throughput = 1
primaryLobe = none

for bounce:
    sample = SampleBSDF(surface, outgoingDirection, rng)
    if sample.invalid: break
    if bounce == 0: primaryLobe = sample.lobe

    throughput *= sample.f * sample.cosTheta / sample.pdf
    trace ray

    if miss:
        AddToPrimaryLobe(primaryLobe, throughput * SkyRadiance(ray.direction))
        break

    hit = ReconstructSurface(ray)
    AddToPrimaryLobe(primaryLobe, throughput * DirectLightingAtHit(hit, -ray.direction))
    AddToPrimaryLobe(primaryLobe, throughput * EmissiveAtHit(hit, -ray.direction))

    maybe russian roulette
    surface = hit
```

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Single-bounce output matches the previous diffuse/specular passes within expected sampling variance.
- Multi-bounce paths can include diffuse-after-specular and specular-after-diffuse.
- Reference mode can run more than one bounce without needing non-physical intensity multipliers.
- `IndirectDiffuse.hlsl` and `IndirectSpecular.hlsl` both call the same BSDF/path sampling code.
- No new path sampling code is named after only diffuse or only specular unless it truly supports only that lobe.
- Reference compliance: throughput, lobe PDF, Russian roulette, miss handling, and first-lobe output split are mapped to PBRT/RTXPT/Falcor behavior.
- Reuse/DRY: diffuse and specular indirect passes share one canonical path-surface, path-sample, PDF, throughput, and hit/miss resolve implementation.

## Stage 8: Add Secondary Shadowing and Direct-Light Sampling at Hits

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Make `ShadeRayTracingHitIncidentRadiance` optionally shadow secondary-hit direct lights. Start with one shadow ray to the sampled/selected light direction, then add explicit light sampling if the path sampler supports next-event estimation with PDFs.

Reference lineage:

- PBRT next-event estimation and direct lighting at path vertices: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- NVIDIA RTX Path Tracing SDK direct lighting at ray-hit points: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor path tracer direct-light sampling patterns: <https://github.com/NVIDIAGameWorks/Falcor>
- NVIDIA RTXDI integration guide if direct-light reservoir sampling is introduced later: <https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md>

Files:

- `Engine/Assets/Shaders/RayTracing/RayTracingHitLighting.hlsli`
- `Engine/Assets/Shaders/RayTracing/Shadows/RayTracedShadowTrace.hlsli`
- Shared light sampling include if introduced.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Indirect lighting darkens correctly behind occluders at secondary hit points.
- Secondary shadowing can be disabled for performance with a visible debug flag.
- Shadow alpha-test behavior matches primary direct shadows.
- No double-counting of primary direct lighting occurs.
- Reference compliance: next-event/direct-hit lighting equations and shadow-visibility policy are compared against PBRT/RTXPT/Falcor.
- Reuse/DRY: secondary hit lighting calls the same punctual-light, shadow, alpha-test, and material helpers as primary direct lighting.

## Stage 9: Add Environment Importance Sampling

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Build an environment-map luminance distribution for the sky texture and sample it for indirect lighting. This is basic environment importance sampling, not ReSTIR. Combine with BSDF sampling using MIS when both strategies are active.

Reference lineage:

- PBRT infinite area light distribution and PDF handling: <https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights>
- NVIDIA RTX Path Tracing SDK environment sampling: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor environment-map sampling/path tracer references: <https://github.com/NVIDIAGameWorks/Falcor>
- Google Filament IBL/environment-map treatment: <https://google.github.io/filament/Filament.md.html>

Files:

- Sky/environment texture manager or preprocess stage.
- New environment sampling data buffers.
- Indirect diffuse/specular shared path sampling code.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- HDR sky with small bright sun converges faster than cosine-only sampling.
- MIS weights are computed from the same PDFs used by BSDF and environment sampling.
- Energy remains consistent with the selected sky radiance and BSDF throughput policy.
- Miss rays still use the same `SampleSkyEnvironmentRadiance` function.
- Reference compliance: environment PDF, luminance measure, solid-angle weighting, and MIS weights are documented against PBRT/RTXPT/Falcor.
- Reuse/DRY: sky miss radiance and environment importance sampling share one sky orientation/radiance convention and one environment metadata owner.

## Stage 10: Add Emissive Geometry Sampling

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Keep emissive-on-hit support, but add optional explicit emissive triangle/light sampling for high-variance emissive scenes. This can remain behind a reference or high-quality flag until acceleration structures for emitters are mature.

Reference lineage:

- PBRT area/emissive light sampling and direct-light estimators: <https://pbr-book.org/4ed/Light_Sources/Area_Lights>
- NVIDIA RTX Path Tracing SDK emissive/mesh light sampling references: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor path tracer emissive light sampling references: <https://github.com/NVIDIAGameWorks/Falcor>
- Unreal Engine path tracer emissive material behavior as artist-facing reference: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>

Files:

- Ray tracing hit data or scene light extraction.
- Shared light sampling include.
- Indirect path sampling code.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- An emissive panel lights nearby diffuse surfaces without requiring only random ray hits.
- Explicit emitter sampling and hit-only estimation share emitted-radiance decode and double-counting policy.
- Emissive geometry does not get double counted when directly visible to a path ray.
- Reference compliance: emitter selection PDF, area measure, emitted radiance, and MIS/double-counting policy are documented against PBRT/RTXPT/Falcor.
- Reuse/DRY: emissive-on-hit and explicit emissive sampling share one emissive-material radiance decode and one emitter list/selection owner.

## Stage 11: Prepare DLRR or Other Indirect Reconstruction Inputs

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Extend the provider-neutral reconstruction contract for indirect lighting. Do not wire DLRR directly into shader code. First expose the resources and metadata a provider needs.

Reference lineage:

- NVIDIA Streamline DLSS Ray Reconstruction programming guide: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA NRD signal and denoiser input conventions: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA RTX Path Tracing SDK denoiser/reconstruction-facing signal conventions: <https://github.com/NVIDIA-RTX/RTXPT>
- Existing Sparkle upscaler/provider contract must be reused as the provider boundary.

Required resources:

- noisy composed color or noisy indirect color at the provider-required stage
- depth and depth convention
- motion vectors and units
- normals
- roughness
- base color/diffuse albedo
- metallic/F0/specular albedo
- hit distance for specular and/or indirect rays
- exposure or auto-exposure state
- reset/history state

Files:

- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.h`
- Provider contract docs
- Render product registration
- Indirect pass output resources
- Future Streamline DLRR provider implementation

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- DLSS Super Resolution path still works unchanged.
- A DLRR-capable provider can be selected only when all required resources are available.
- Missing resources fail provider selection at the contract boundary instead of silently falling back to an invalid reconstruction path.
- Reference compliance: each provider input resource is mapped to Streamline DLRR, NRD, or RTXPT signal expectations, with units/ranges documented.
- Reuse/DRY: DLRR/reconstruction integration extends the existing provider contract and render-product registration instead of creating a parallel provider system.

## Stage 11A: Add Indirect Denoiser Auxiliary Signal Buffers

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Refactor indirect diffuse/specular output so denoisers and reconstruction providers can consume stable auxiliary signals instead of only final noisy lighting. The real-time path may still composite direct/indirect buffers the current way, but provider-facing buffers must describe the noisy path sample, material factors, geometry, and confidence needed to reconstruct lighting correctly.

Reference lineage:

- NVIDIA Streamline DLSS Ray Reconstruction programming guide for required resource tags such as noisy color, diffuse albedo, specular albedo, normal, roughness, depth, motion, and specular hit distance: <https://github.com/NVIDIAGameWorks/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA NRD REBLUR/RELAX signal conventions for demodulated radiance, hit distance, roughness, normals, viewZ, motion, and history: <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA RTXPT/Falcor path tracing denoiser-output structure and debug passes.
- AMD FidelityFX denoiser/upscaler examples for temporal resource contracts.
- Existing Sparkle `UpscalerInputContract` and frame-graph product registration must be extended rather than bypassed.

Files:

- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- Future shared path-sampling files from Stage 7.
- `Engine/Renderer/Private/FrameGraph/Resources/*`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.*`
- Provider-neutral reconstruction/denoiser interfaces.

Required buffers or views:

- Noisy indirect diffuse radiance and noisy indirect specular radiance, or a provider-approved composed noisy color split.
- Demodulated radiance when the selected denoiser expects material factors to be applied after denoising.
- Diffuse albedo, specular albedo/F0, roughness, metallic or lobe metadata, and material validity flags.
- First-bounce hit distance for diffuse/specular rays and specular hit distance for DLRR-style reconstruction.
- Shading normal or provider-required normal space, depth/viewZ, motion vectors, exposure, confidence/variance, lobe id, and history reset.
- Provider-facing buffers have one documented writer and one documented format/space owner.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- Indirect passes can output provider-neutral auxiliary buffers without changing lighting math.
- DLRR/NRD-capable provider selection requires all auxiliary buffers to exist and match documented formats.
- Reconstruction can be disabled without changing the real-time lighting math or duplicating auxiliary writers.
- Reference compliance: each auxiliary signal is mapped to Streamline DLRR, NRD, RTXPT/Falcor, or AMD FidelityFX expectations and units.
- Reuse/DRY: indirect diffuse, indirect specular, reference mode, DLRR, and NRD share the same auxiliary signal writers instead of each effect inventing a private layout.

## Stage 12: Add a High-Sample Reference Mode

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Add a reference path tracing mode or offline accumulation path that uses the same material, light, sky, and alpha-test code as the real-time passes. The reference mode can be slow. It exists to define truth.

Reference lineage:

- PBRT path tracer as correctness reference: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- NVIDIA RTX Path Tracing SDK real-time path tracer structure: <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor path tracer/reference-rendering infrastructure: <https://github.com/NVIDIAGameWorks/Falcor>
- Unreal Engine path tracer for production reference-mode expectations: <https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine>

Files:

- New path tracing shader/pass, or a reference mode in existing indirect path infrastructure.
- Existing presentation/output paths only if a high-sample result needs a visible target.
- Reference-mode contract notes under `Docs/Rendering/PBR`.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- The reference mode can accumulate many samples per pixel into a linear HDR radiance target.
- The same scene data can be shaded by real-time direct/indirect and reference path-traced paths without duplicate material/light/sky code.
- Direct, indirect diffuse, indirect specular, and composed scene color have clear reference-mode equivalents.
- Reference compliance: reference-mode equations, sampling, and accumulation are documented against PBRT/RTXPT/Falcor/Unreal.
- Reuse/DRY: reference mode shares material decode, BRDF, light units, sky radiance, alpha-test, and path sampling modules with the real-time path wherever possible.

## Stage 13: Final PBR Architecture and Cleanup Gate

Implementation prompt:

Prompt guardrail: include `Reference lineage` and `Reuse/DRY audit`; scan existing bodies before adding or moving logic; simplify first by reusing existing homes, deleting stale code, and justifying any new layer. Do not add wrappers that only rename or forward parameters; keep a helper only when it owns real policy, math, IO binding, repeated behavior, or a meaningful boundary. Improve unclear names while touching code, and remove needless logging, debug noise, indirection, and complexity before adding new logic.

Run a final cleanup and architecture pass over the PBR work before treating it as a foundation. This gate removes temporary toggles, diagnostic leftovers, stale docs, duplicate helpers, debug-only indirection, unowned signal descriptions, and pass-local forks that accumulated during implementation. It is not a request to build a new validation framework.

Reference lineage:

- PBRT reference path tracing equations: <https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer>
- NVIDIA RTXPT and Falcor reference/path-tracing organization.
- NVIDIA NRD denoiser signal ownership expectations.
- NVIDIA Streamline DLSS Ray Reconstruction provider-resource tagging.
- AMD FidelityFX SDK temporal resource patterns.
- Unreal Engine path tracer as artist-facing ground-truth comparison reference.

Correctness surfaces to audit, using existing scenes or minimal fixtures only when needed:

- White furnace: diffuse white, dielectric, rough metal, mirror metal, and subsurface/off-reference variants.
- Mirror sky: perfect mirror reflecting HDR sky before presentation.
- glTF punctual light calibration: known point/spot/directional units and distances.
- Alpha card shadows: cutout geometry in primary direct shadows and secondary rays.
- HDR sky with small bright sun: environment miss and importance-sampling convergence.
- Emissive panel: hit-only vs explicit emitter sampling.
- Rough metal/specular hit distance: indirect specular denoiser inputs.
- Motion/history: camera cut, object motion, resize, and jitter reset.
- Tone mapping/exposure: black, gray, HDR ramp, bright sky, non-power-of-two extent.

Required cleanup outcomes:

- Stale debug-only paths, unused feature toggles, redundant wrappers, obsolete docs, and duplicate signal descriptions are removed or explicitly justified.
- Pre-tonemap HDR scene color, direct lighting, indirect diffuse, indirect specular, sky, emissive, shadow visibility, shadow hit distance, denoiser auxiliary buffers, exposure, and final presentation keep one owner for format/space/unit semantics.
- Any remaining local deviation from PBRT/Filament/Unreal/RTXPT/Falcor/NRD/Streamline/AMD is named in the owning stage, not hidden in code comments.
- Each completed implementation stage records references followed, files changed, deviations, and reuse/DRY cleanup result.

Acceptance criteria:

- Simplification gate: remove needless wrappers, unclear names, stale logging/debug noise, duplicate indirection, and avoidable complexity introduced or exposed by the stage; any helper kept must own real policy, math, IO binding, repeated behavior, or a meaningful boundary.
- No PBR stage leaves behind a duplicate helper, stale wrapper, pass-local fork, or doc-only contract that conflicts with source ownership.
- Tone mapping happens once, sky remains HDR before presentation, F0 matches between primary/ray-hit shading, shadows honor alpha testing, and denoiser buffers have valid units/ranges by construction of the owning modules.
- Reference compliance: final contracts are tied to PBRT/RTXPT/Falcor/NRD/Streamline/AMD/Unreal references and local documented deviations.
- Reuse/DRY: reference mode, provider paths, and real-time passes reuse the same material, BRDF, light, sky, visibility, and signal ownership code where possible.

## Recommended Implementation Order

Do these first:

1. Stage 0E: repository-wide PBR audit gate.
2. Stage 0A: shader module boundaries.
3. Stage 0B: thin pass entrypoints.
4. Stage 0F: render target, precision, and signal surface contracts.
5. Stage 0C: display transform and exposure metering.
6. Stage 0D: audit and refine existing exposure/tone mapping.
7. Stage 1: linear HDR sky transport.
8. Stage 2A: asset color-space, material extension, and import unit rules.
9. Stage 2B: geometry, normal, depth, motion, and temporal conventions.
10. Stage 2: material F0 and roughness consistency.
11. Stage 2C: full roughness range and reference roughness policy.
12. Stage 3A: centralized shading-model and lobe energy policy.
13. Stage 4: light units/falloff.
14. Stage 4A: finite light source and soft-shadow approximation policy.
15. Stage 5: alpha-tested shadows, raw visibility, hit distance, and signal format split.
16. Stage 7: unified BSDF path sampler.

Then:

17. Stage 8: secondary shadowing.
18. Stage 9: environment importance sampling.
19. Stage 10: emissive geometry sampling.
20. Stage 6: NRD SIGMA shadow integration.
21. Stage 11: DLRR resource contract.
22. Stage 11A: indirect denoiser auxiliary signal buffers.
23. Stage 12: high-sample reference mode.
24. Stage 13: final PBR architecture and cleanup gate.

Shader cook/build checks should run throughout. Stage 13 should start early as cleanup opportunities appear, then close at the end as the architecture gate.

## Definition of Done for "PBR Correct Enough to Build On"

The renderer is on a credible ground-truth path when all of these are true:

- Sky and lighting are linear HDR until presentation.
- Direct primary and ray-hit direct use the same BRDF, material F0, full-range roughness policy, and light units.
- Roughness `0.0` through `1.0` is preserved as material data and evaluated/traced without hidden roughness floors.
- Punctual light falloff and spot cones are documented and calibrated.
- Alpha-tested geometry participates in direct shadows and indirect ray hits.
- Indirect throughput is always `BSDF * cosine / pdf`.
- Multi-bounce indirect can sample both diffuse and specular lobes.
- Secondary hit direct lighting can be shadowed.
- Raw denoiser inputs exist separately from final lighting targets.
- Denoiser/reconstruction signal formats match the shader data written into them.
- Asset import/cooking paths enforce color-space, packed-channel, HDR sky, and light-unit contracts.
- Normal, depth/viewZ, motion-vector, jitter, and history-reset conventions are locked at the provider boundary.
- Real-time output and high-sample reference mode share material, BRDF, light, sky, and path sampling code wherever practical.
- Final stage notes include references followed, known deviations, and reuse/DRY cleanup evidence.
- Reusable lighting/ray-tracing shader modules do not depend on pass-specific deferred files.
- Pass entrypoints are small orchestration layers over concept-named modules.
- New lighting features reuse shared light, surface, path, visibility, and sky code instead of forking effect-local copies.
- Every completed stage records its exact reference lineage and reuse/DRY audit.
