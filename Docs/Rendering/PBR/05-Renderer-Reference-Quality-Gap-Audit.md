# Sparkle Renderer Reference Quality Gap Audit

This audit is the active implementation-prompt source for getting Sparkle closer to reference-quality real-time rendering. It intentionally focuses on the two paths that matter for editor-visible progress:

1. RTXDI/ReSTIR-style many-light direct lighting.
2. Streamline DLSS Ray Reconstruction for noisy path/ray-traced lighting.

Direct-light quality should come from many-light sampling, reservoir reuse, physically sampled area lights, and correct final weighted shading.

## Reference Lineage

- NVIDIA RTXDI: ReSTIR DI / ReGIR / ReSTIR GI provide the reference many-light sampling structure. The application owns light buffers, material/GBuffer/ray-tracing bridge code, resource allocation, reservoir products, selected-sample visibility, and final shading. <https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md>
- NVIDIA RTXDI overview: RTXDI targets dynamic direct lighting and shadows from many lights with small ray budgets through spatiotemporal importance resampling. <https://developer.nvidia.com/blog/lighting-scenes-with-millions-of-lights-using-rtx-direct-illumination/>
- NVIDIA Streamline DLSS Ray Reconstruction: DLRR requires provider-tagged noisy HDR color plus guide buffers such as diffuse albedo, specular albedo, normals, roughness, motion vectors, depth, and specular hit distance or specular motion vectors. <https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA RTXPT: path tracing structure, many-light sampling, guide-buffer generation, and Streamline integration are useful architecture references. <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor PathTracer: path tracing is a separate render pass/render graph owner for path generation, direct-light sampling at path vertices, accumulation, and optional reconstruction wiring. <https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md>
- AMD neural reconstruction: noisy color plus guide buffers such as albedo, normal, roughness, depth, and specular hit distance feed a distinct reconstruction stage. <https://gpuopen.com/learn/neural_supersampling_and_denoising_for_real-time_path_tracing/>

## Universal Implementation Rules

Every stage below must leave the touched code simpler than it found it.

- Scan existing bodies before adding or moving logic.
- Start with code denoising: delete stale scaffolding, fake provider paths, needless logging, duplicate docs, and wrappers that only rename or forward parameters.
- Keep a helper only when it owns real policy, math, IO binding, resource lifetime, repeated behavior, or a meaningful architecture boundary.
- Follow NVIDIA/AMD reference patterns concretely enough that the shape is recognizable.
- Avoid backend-specific shaders or renderer-facing backend branches. Backend/native details belong in RHI/provider bridges.
- Preserve current visible behavior unless the stage explicitly changes it and names the editor-visible result.
- If a stage needs a new layer, justify why existing homes cannot own the behavior.

## Current Source-Backed State

| Area | Current source | Assessment |
| --- | --- | --- |
| Render path selection | `FrameRenderPath`, `BuildFrame`, `FrameBuildResult`, `FrameGraphBuildResult`, `FramePipeline` | Good direction. The built graph reports the path it contains. |
| Reference path ownership | `Frame/Reference/*`, `Passes/Reference/ReferencePathTracingPass.*`, `Passes/Reference/ReferencePathTracing.hlsl` | Good direction. Reference targets are no longer lighting targets. |
| Direct shadow visibility | `Frame/Lighting/ShadowVisibility.cpp`, `Frame/Lighting/Shadows/DirectShadowSignal.cpp`, `DirectShadowSignal.hlsl`, `RayTracedShadowSignalPacking.hlsli` | Stage 6M writes raw visibility/hit-distance before direct lighting and pairs it with `DirectShadowLightSample` identity/PDF. |
| Direct lighting | `DirectLighting.hlsl`, `DirectLightingPass.*`, `DirectShadowSignalPass.*` | Direct lighting is backend-agnostic and consumes sampled-light plus visibility products. It no longer traces shadows inline or has direct-lighting ray-query/no-ray/device-address shader variants. |
| Many-light sampling | `DirectLightSampling.hlsli`, `LightSampling.hlsli`, `AreaLights.hlsli` | Initial one-candidate-per-pixel slice exists. Next step is temporal/spatial reservoir reuse or an RTXDI provider boundary. |
| Upscaler provider | `UpscalerInputContract`, `NvidiaDlssUpscalerProvider`, `StreamlineDlssRuntime` | DLSS-SR/DLAA shaped. Keep separate from DLRR. |
| Reconstruction resources | `FrameAssemblyRayReconstructionProviderResources`, `FrameProviderInputs`, `LightingRenderTargets` guide slots | Useful vocabulary, but not yet a complete provider contract and not tagged/evaluated by a reconstruction provider. |
| Physical lighting contract | `SurfaceLighting`, `AreaLights`, `PathSampling`, `PathLighting`, lighting target contract docs | Good direction. Main remaining risk is direct-light sampling stability and reconstruction signal correctness. |

## P0 Decisions

### P0.1 Direct Lighting Must Be RTXDI/ReSTIR-Shaped

Current state:

- Stage 6M deleted the first-shadow-casting-light shortcut.
- `DirectLightSampling.hlsli` samples one contribution-weighted candidate per pixel across represented directional, point, spot, and rect lights.
- `DirectShadowSignal` writes raw visibility/hit-distance for that candidate.
- `DirectLighting` shades from the sampled-light product and paired visibility signal.

Decision:

- Do not add a visibility-only post-filter path as the direct-lighting architecture.
- Upgrade the current one-sample slice into RTXDI/ReSTIR-style direct lighting:
  - initial candidate generation over all represented lights
  - reservoir identity, weight, target PDF, and confidence only where needed
  - temporal reservoir reuse with explicit history ownership and reset
  - spatial reservoir reuse with bounded neighborhood policy
  - selected-sample visibility tied to reservoir identity
  - final direct-light shading using reservoir weight/PDF

Reference mapping:

- RTXDI/ReSTIR DI is the direct-lighting reference: sample/resample many lights, trace selected visibility, shade with reservoir weights.
- RTXPT/Falcor keep direct-light sampling and visibility as explicit path/render-graph products.

### P0.2 DLRR Needs A Reconstruction Contract, Not Upscaler Sprawl

Current state:

- `UpscalerInputContract` is correct for DLSS-SR/DLAA.
- DLRR needs a wider noisy-lighting and guide-buffer contract.
- Current guide slots are not enough for provider selection or Streamline resource tagging.

Decision:

- Add `RayReconstructionInputContract` separate from `UpscalerInputContract`.
- Add provider category `RayReconstruction`. Do not overload `Upscaler`.
- Build DLRR on top of Streamline while keeping the Sparkle-side contract separate:
  - noisy HDR color or noisy indirect color
  - output color
  - diffuse albedo
  - specular albedo/F0
  - normal and roughness
  - depth with exact convention metadata
  - motion vectors and scale
  - specular hit distance or specular motion vectors
  - exposure and reset state
  - render/output extent and frame token metadata

Reference mapping:

- Streamline DLRR requires tagged resources and common constants. It can share low-level Streamline runtime primitives with DLSS-SR, but it is not the same provider contract.

### P0.3 Reference Path Must Write Its Own Guides

Current state:

- Reference path tracing owns primary rays.
- Realtime GBuffer products are invalid as reference guide buffers unless a reference pass explicitly writes equivalent products.

Decision:

- Treat reference reconstruction as path-tracer-owned guide generation.
- If reference mode wants reconstruction, `ReferencePathTracing` or a sibling reference-guide pass must output:
  - primary hit depth/viewZ
  - shading normal
  - roughness
  - diffuse/specular albedo
  - hit distance
  - motion vectors if temporal reconstruction is enabled
- Do not make shared post/provider code silently pull realtime GBuffer handles in reference mode.

Reference mapping:

- RTXPT and Falcor keep path tracing as the owner of path generation, path outputs, and path guide buffers.

## Stages

### Stage 6X: Add RTXDI-Style Reservoir Reuse For Direct Lighting

Implementation prompt:

Prompt guardrail: apply the universal implementation rules above. Start by denoising the Stage 6M sampled-light path: remove temporary one-sample naming, keep `DirectLightSampling` only for candidate policy, and do not add wrappers that merely rename reservoirs or light samples.

Goal:

- Move from a one-sample vertical slice to an industry-standard RTXDI/ReSTIR DI direct-lighting path for many lights.

Existing bodies to scan first:

- `Engine/Assets/Shaders/Lighting/DirectLightSampling.hlsli`
- `Engine/Assets/Shaders/Lighting/LightSampling.hlsli`
- `Engine/Assets/Shaders/Lighting/AreaLights.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectShadowSignal.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Renderer/Private/Frame/Lighting/ShadowVisibility.*`
- `Engine/Renderer/Private/Frame/Lighting/Shadows/*`
- `Engine/Renderer/Private/Passes/Deferred/DirectShadowSignalPass.*`
- `Engine/Renderer/Private/Passes/Deferred/DirectLightingPass.*`
- `Engine/RHI/Public/Resources/RenderViewLightingData.h`
- Scene light snapshot/build code and any future compact light-table owner.

Required implementation:

- Add reservoir products:
  - initial candidate reservoir over all represented direct lights
  - temporal reservoir reuse with explicit history ownership and reset
  - spatial reservoir reuse with bounded neighborhood policy
  - selected-sample visibility tied to reservoir identity
  - final direct-light shading using reservoir weight/PDF
- Keep light units, area-light sampling, BRDF evaluation, and shadow tracing in their existing concept-owned modules.
- Keep direct lighting backend-agnostic; ray queries remain in the visibility pass.
- Add confidence only if it is consumed by the reservoir path or a real reconstruction provider. Do not add debug-only confidence scaffolding.

Editor result:

- A scene with many shadow-casting lights is stable enough to evaluate visually with one or few visibility rays per pixel.
- Light count changes affect noise and convergence according to the reservoir policy, not by silently dropping lights.

Acceptance criteria:

- Code denoising gate: no temporary first-light code, no filtered-first/raw-rest hybrid, no backend-specific direct-light shaders, no empty reservoir wrappers.
- Direct lighting uses reservoir identity/weight/PDF for final shading.
- Visibility maps to the selected reservoir sample, not an unordered aggregate.
- Temporal/spatial reuse has clear history ownership and reset rules.
- Area-light soft shadows remain physically sampled through light sampling.
- Reference compliance note maps candidate generation, reservoir update/reuse, selected visibility, confidence, and final shading to RTXDI/ReSTIR/RTXPT/Falcor.
- Reuse/DRY note lists reused light sampling, shadow tracing, frame-graph, and scene-light bodies and justifies any new reservoir owner.

### Stage 11R: Add Ray Reconstruction Provider Contract And Guides

Implementation prompt:

Prompt guardrail: apply the universal implementation rules above. Start by denoising the current upscaler/reconstruction boundary: remove fields that pretend DLRR is an upscaler input, collapse duplicate guide descriptions, and avoid a global signal registry. Do not add wrappers that merely repack material values if existing material/GBuffer/path helpers already own the policy.

Goal:

- Make DLRR and future reconstruction providers selectable only when all required resources exist, and write the guide buffers needed for a visible DLRR pass.

Existing bodies to scan first:

- `Engine/Renderer/Private/Upscaling/*`
- `Engine/Renderer/Private/Providers/*`
- `Engine/Renderer/Private/Frame/Core/FrameAssembly.h`
- `Engine/Renderer/Private/Frame/Core/FrameProviderInputs.*`
- `Engine/Renderer/Private/Frame/Lighting/*`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectDiffuse.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/IndirectSpecular.hlsl`
- `Engine/Assets/Shaders/RayTracing/Path*.hlsli`
- `Engine/Assets/Shaders/Material/*`
- `Engine/Assets/Shaders/Passes/Deferred/GBufferUtils.hlsli`

Required implementation:

- Add `RayReconstructionInputContract` separate from `UpscalerInputContract`.
- Add provider category `RayReconstruction` or a precise reconstruction category.
- Convert current indirect reconstruction frame slots into explicit provider input products with one owner and one writer.
- Write provider-ready noisy HDR color, diffuse albedo, specular albedo/F0, normal/roughness, depth, motion, exposure/reset, and specular hit-distance or specular motion-vector resources from the realtime path.
- Keep guide writers near path/lighting code and reuse existing material/F0/albedo helpers.
- Add contract validation for required DLRR inputs and formats at provider selection.
- Keep reconstruction disabled until all required resources exist.

Editor result:

- Reconstruction mode remains unavailable until all required resources exist.
- The editor can show an unreconstructed noisy indirect/specular scene using the same lighting math that DLRR will consume.

Acceptance criteria:

- Code denoising gate: `UpscalerInputContract` remains about upscaling; DLRR guide resources do not sprawl into unrelated structs or duplicate material packing helpers.
- Provider selection can reject missing guide buffers before execution.
- Guide buffers have a single writer and a documented owner.
- Real-time lighting math does not fork for DLRR; it only publishes required guide products.
- No global all-signals namespace or registry is introduced.
- Reference compliance note maps each guide resource to Streamline DLRR, RTXPT/Falcor, or AMD reconstruction expectations.
- Reuse/DRY note lists which material, GBuffer, path, provider, and frame-assembly bodies were reused.

### Stage 11D: Implement Streamline DLRR Provider

Implementation prompt:

Prompt guardrail: apply the universal implementation rules above. Start with a code denoising pass through the existing Streamline DLSS runtime: share only primitives that own real Streamline API policy, and keep DLSS-SR/DLAA separate from DLRR resource contracts. Do not merge DLRR into `UpscalerInputContract` or add DLRR-specific shader code unless a provider requirement genuinely needs it.

Goal:

- Implement DLRR through Streamline without merging it into DLSS-SR code, and expose it as a visible editor reconstruction mode.

Existing bodies to scan first:

- `Engine/Renderer/Private/Upscaling/NvidiaDlss/*`
- Streamline runtime wrappers and resource-tag construction.
- Provider model/category code.
- Stage 11R `RayReconstructionInputContract`.
- RHI native interop services.
- Editor renderer settings panel and renderer settings storage.

Required implementation:

- Query and load `sl::kFeatureDLSS_RR`.
- Add a Streamline DLRR provider separate from the DLSS-SR provider.
- Set DLRR options and common constants from the reconstruction contract and current view/temporal state.
- Tag noisy HDR color, output color, diffuse albedo, specular albedo, normal/roughness, depth, motion vectors, specular hit distance or specular motion vectors, exposure/reset.
- Restore command state after `slEvaluateFeature`.
- Add renderer/editor settings for reconstruction off/DLRR and only the required Streamline quality mode.
- Keep final output in the normal exposure/presentation path.

Editor result:

- A noisy indirect/specular scene can toggle unreconstructed output versus DLRR output.
- The viewport output remains explicit: final linear HDR lighting goes through the same exposure and presentation path after reconstruction.

Acceptance criteria:

- Code denoising gate: DLSS-SR code does not grow DLRR-only fields, and DLRR code does not duplicate generic Streamline runtime primitives.
- DLRR mode is unavailable unless Streamline feature availability and all required resources are satisfied.
- Resource tags are recognizable against the Streamline DLRR programming guide.
- Command-state restoration is explicit and localized to the Streamline provider runtime.
- DLRR failure falls back to the known unreconstructed path without hidden global state.
- Reference compliance note maps feature query, resource tags, constants, dispatch/evaluate, and fallback behavior to Streamline and AMD-style provider patterns.
- Reuse/DRY note names which DLSS/Streamline primitives were shared and which were intentionally kept separate.

Implementation note:

- Implemented as a separate ray-reconstruction provider path: `RayReconstructionInputContract`, `RayReconstructionSubsystem`, `RayReconstructionEvaluation`, and `NvidiaDlssRayReconstructionProvider`. `UpscalerInputContract` remains DLSS-SR/DLAA-only.
- Streamline mapping follows the DLRR programming guide: `sl::kFeatureDLSS_RR` feature load/query, `slDLSSDSetOptions`, shared `sl::Constants`, frame resource tags for noisy HDR color, output color, albedo/specular albedo, normals, roughness, depth, motion vectors, specular hit distance, and exposure, then `slEvaluateFeature`.
- AMD-style provider staging is preserved: noisy signal generation and reconstruction are distinct graph stages, and failure falls back to the unreconstructed HDR color before exposure/presentation.
- Shared primitives: Streamline texture resource/subresource conversion and view constants are common to DLSS-SR and DLRR. Kept separate: provider contracts, provider runtimes, settings, and frame evaluation passes.
- Local deviation to resolve in Stage 11R follow-up: roughness is currently tagged from the material-guide product's roughness channel rather than from a dedicated single-channel roughness product. The provider contract has a named roughness slot so this can be split without changing the DLRR provider boundary.

### Stage 12G: Reference Path Guide Outputs

Implementation prompt:

Prompt guardrail: apply the universal implementation rules above. Start by denoising reference/realtime guide boundaries: remove any code or docs implying reference mode may borrow realtime GBuffer handles. Do not add wrappers that only mirror realtime guide structs unless the reference path truly writes equivalent data.

Goal:

- Make path-traced reference outputs comparable to realtime and provider-ready without borrowing GBuffer products.

Required implementation:

- Add optional reference guide targets for primary depth/viewZ, normal, roughness, albedo/F0, first-hit distance, and path lobe metadata.
- Keep guide targets, allocation, and pass scheduling under `Frame/Reference`.
- Let reference reconstruction consume those guides explicitly.
- Do not publish realtime GBuffer products from reference mode unless a reference guide pass actually writes equivalent products.
- Reuse shared material, BRDF, sky, light, alpha-test, and path-sampling code; do not copy reference-only versions of those policies.

Acceptance criteria:

- Code denoising gate: reference path code contains no fake GBuffer handles, no duplicated material/light policy, and no guide wrappers that do not write actual guide data.
- Reference guide resources are owned by `Frame/Reference` and have one writer.
- Shared path/material/light modules remain shared between realtime and reference paths.
- Provider/reconstruction code selects reference guides only when the reference path produced them.
- Reference compliance note maps guide ownership to RTXPT/Falcor and provider guide expectations to Streamline/AMD.
- Reuse/DRY note lists reused shader modules and any intentional local deviations.

Implementation note:

- Reference path tracing now writes its own guide targets under `Frame/Reference`: primary device depth, primary normal, primary diffuse albedo, primary specular albedo/F0, primary material guide, and primary path sample guide.
- Reused shader modules: `RayTracing/PathLighting`, `RayTracing/PathSampling`, `RayTracing/RayTracingMaterialHit`, `Lighting/SurfaceLighting`, `Lighting/AreaLights`, `Lighting/SkyEnvironment`, and `RayTracing/Shadows/RayTracedShadowVisibility`.
- Reference reconstruction uses the shared ray-reconstruction provider pass only when all required provider resources exist. Current intentional blocker: reference mode does not yet write a real reference motion-vector guide, so DLRR is not scheduled from reference mode rather than borrowing realtime GBuffer motion vectors.
- Reference mapping: guide ownership follows RTXPT/Falcor path-tracer-owned output products; provider gating follows Streamline/AMD reconstruction staging where incomplete guide sets must not execute.

## What To Delete Or Avoid

- Do not add a shadow-only provider path for direct lighting.
- Do not keep aggregate shadow signals as a long-term direct-lighting input.
- Do not extend `UpscalerInputContract` into a DLRR kitchen sink.
- Do not add backend-specific shaders for provider work. Backend-specific interop belongs in provider runtime/RHI bridges.
- Do not make reference mode publish realtime GBuffer products unless a reference guide pass actually writes them.
- Do not add wrappers that only rename a comparison or forward a struct.

## Suggested Priority Order

1. Stage 6X: add temporal/spatial reservoir reuse or an RTXDI provider boundary for industry-standard many-light direct lighting.
2. Stage 11R: add `RayReconstructionInputContract`, provider category, and realtime guide buffers needed by DLRR.
3. Stage 11D: add Streamline DLRR provider and editor-visible ray reconstruction.
4. Stage 12G: make reference path write its own guides after realtime RTXDI/DLRR results are visible.
5. Final cleanup: collapse stale shadow/reconstruction docs and remove obsolete future wording after provider integrations land.

## Bottom Line

The direct-lighting target is RTXDI/ReSTIR-style many-light sampling. Sparkle should spend complexity budget on reservoirs, selected-sample visibility, correct PDFs/weights, physical area-light sampling, DLRR guide buffers, and Streamline DLRR integration.
