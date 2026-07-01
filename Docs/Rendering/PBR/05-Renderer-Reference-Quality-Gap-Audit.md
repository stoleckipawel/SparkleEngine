# Sparkle Renderer Reference Quality Gap Audit

This audit compares the current Sparkle renderer architecture against NVIDIA NRD SIGMA, Streamline DLSS Ray Reconstruction, RTXPT, Falcor PathTracer, and AMD neural denoising/upscaling patterns. It is intentionally decision-oriented: if a current path blocks physical correctness or reference-quality integration, the recommendation is to replace it rather than preserve compatibility.

## Reference Lineage

- NVIDIA NRD: API-agnostic spatio-temporal denoisers for low-spp signals, using GBuffer guides such as normal, roughness, viewZ, and motion vector. SIGMA is the shadow-only denoiser. NRD does not allocate resources; the application owns texture creation and dispatches the compute passes from NRD descriptors. <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA Streamline DLSS Ray Reconstruction: DLRR requires provider-tagged noisy HDR color plus guide buffers such as diffuse albedo, specular albedo, normals, roughness, motion vectors, depth, and either specular motion vectors or specular hit distance. It is an extension around DLSS evaluation, but its resource contract is much broader than DLSS Super Resolution. <https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA RTXPT: pure path tracer, no raster dependency in the main configuration, with reference and real-time modes, path-space layer decomposition, guide-buffer generation, NRD, and Streamline integration. <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor PathTracer: separate render pass/render graph. It can use lightweight primary visibility input, then owns path generation, path tracing, direct-light sampling at path vertices, accumulation, and optional denoiser graph wiring. <https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md>
- AMD neural supersampling/denoising: real-time path tracing reconstruction benefits from noisy color plus guide buffers such as albedo, normal, roughness, depth, and specular hit distance, with denoising and upscaling treated as explicit reconstruction stages. <https://gpuopen.com/learn/neural_supersampling_and_denoising_for_real-time_path_tracing/>

## Executive Verdict

Sparkle is now structurally pointed in the right direction: `FrameRenderPath` separates realtime deferred rendering from path-traced reference rendering, reference targets live in `Frame/Reference`, linear HDR is preserved into presentation, and reconstruction resources are not hidden inside the DLSS-SR upscaler contract.

The renderer is not yet at NVIDIA-reference integration quality. The largest blockers are:

1. SIGMA cannot improve final direct lighting yet because direct lighting currently traces and applies raw visibility inside the same pass that writes the raw shadow signal.
2. The raw shadow signal is an aggregate per pixel, not a provider-ready per-light or selected-light SIGMA input.
3. DLRR is not a provider path yet; the existing Streamline runtime only evaluates DLSS Super Resolution / DLAA.
4. Reconstruction guide buffers exist as frame assembly slots, but there is no provider-neutral reconstruction contract or Streamline DLRR tag path.
5. Reference path tracing owns primary rays, but provider guide buffers are still mostly realtime-GBuffer-shaped. Reference mode must write its own guides if it wants denoising/reconstruction.

## First Target: Editor-Visible SIGMA And DLRR

The next work should produce visible editor results before broad cleanup continues. The first target is not a perfect final provider architecture; it is a clean, reference-shaped vertical slice that can be switched on in the editor and judged visually.

Priority:

1. Finish SIGMA shadow denoising end-to-end: raw shadow signal before lighting, NRD provider execution, persistent provider history, editor setting, and direct lighting consuming raw or denoised visibility.
2. Finish DLRR/ray reconstruction end-to-end: provider-neutral reconstruction contract, guide buffers, Streamline DLRR resource tags, editor setting, and reconstructed output visible in the viewport.
3. After those are visible, return to reference-path guide outputs and deeper renderer architecture cleanup.

Editor-visible definition of done:

- A renderer settings panel exposes shadow visibility mode as raw or NRD SIGMA only when an NRD provider is available.
- A noisy low-sample shadow scene visibly changes between raw visibility and SIGMA denoised visibility without changing light energy.
- A renderer settings panel exposes reconstruction mode as off or Streamline DLRR only when the Streamline provider can satisfy the required tags.
- A noisy indirect/specular reconstruction scene visibly changes between unreconstructed and DLRR output while preserving linear HDR lighting before presentation.
- Unsupported provider modes are not shown as working features; they remain absent or disabled with a short reason at the provider boundary.

## Current Source-Backed State

| Area | Current source | Assessment |
| --- | --- | --- |
| Render path selection | `FrameRenderPath`, `BuildFrame`, `FrameBuildResult`, `FrameGraphBuildResult`, `FramePipeline` | Good direction. The built graph now reports the path it actually contains. |
| Reference path ownership | `Frame/Reference/*`, `Passes/Reference/ReferencePathTracingPass.*`, `Passes/Reference/ReferencePathTracing.hlsl` | Good direction. Reference targets are no longer lighting targets. |
| Shadow visibility signal | `Frame/Lighting/ShadowVisibility.cpp`, `DirectLighting.hlsl`, `RayTracedShadowDenoiserInputs.hlsli` | Honest raw-visibility path. No SIGMA mode is exposed until a real provider exists. |
| Direct shadows | `DirectLighting.hlsl`, `DirectLightingPass.*` | Physically usable for raw visibility, but architecturally wrong for SIGMA because visibility is applied before denoising. |
| Shadow history | Not allocated in the current runtime | Correct current state. Future SIGMA history must be introduced by the provider pass that consumes it. |
| Upscaler provider | `UpscalerInputContract`, `NvidiaDlssUpscalerProvider`, `StreamlineDlssRuntime` | DLSS-SR/DLAA shaped. This should stay separate from DLRR. |
| Reconstruction resources | `FrameAssemblyDenoiserProviderResources`, `FrameProviderInputs`, `LightingRenderTargets` guide slots | Good vocabulary, but not a complete provider contract and not tagged/evaluated by any provider. |
| Physical lighting contract | `SurfaceLighting`, `AreaLights`, `PathSampling`, `PathLighting`, lighting target contract docs | Much improved. Main remaining risk is denoiser/reconstruction signal correctness, not basic radiance units. |

## P0 Decisions

These are required before claiming SIGMA or DLRR-quality integration.

### P0.1 Split Direct Visibility From Direct Lighting Accumulation

Current problem:

- `DirectLighting.hlsl` traces a direct-light sample, immediately multiplies lighting by `shadow.Visibility`, and also writes `ShadowVisibilitySignalTexture`.
- A denoiser can only improve a signal that is consumed after denoising. Today, direct lighting has already used raw stochastic visibility.
- The current raw shadow output is an aggregate "most occluding" per-pixel signal across all direct light samples. SIGMA is per-light shadow denoising; an aggregate visibility/hit-distance value cannot reconstruct multiple lights correctly.

Decision:

- Replace the current one-pass direct shadow path with an explicit shadow signal path before final direct-light accumulation:
  1. `DirectShadowSignal` pass samples a light or light set and writes provider-ready visibility/hit-distance.
  2. Optional `ShadowDenoise` provider pass consumes raw shadow signal plus normal/depth/viewZ/motion/history and writes denoised visibility.
  3. `DirectLighting` consumes either raw or denoised visibility through an explicit visibility source.

File direction:

- Keep light/BRDF evaluation in `Lighting/SurfaceLighting.hlsli` and `Lighting/AreaLights.hlsli`.
- Move shadow signal generation out of `DirectLighting.hlsl` into a concept-owned shadow pass under `Frame/Lighting/Shadows` and shader code under `RayTracing/Shadows`.
- `DirectLighting.hlsl` should become a radiance accumulation pass, not the owner of denoiser input generation.

Reference mapping:

- NRD SIGMA expects shadow signal and guide inputs before denoising.
- RTXPT/Falcor keep direct-light sampling and denoiser-facing outputs as explicit path/render-graph products.

### P0.2 Do Not Claim SIGMA Until There Is A Real NRD Provider

Current problem:

- Sparkle has no linked NRD provider execution path.
- The previous local SIGMA CVar/contract/history scaffold did not execute a provider and has been removed.
- The current raw shadow output remains an aggregate "most occluding" per-pixel signal, not a provider-ready per-light SIGMA input.

Decision:

- Keep raw visibility as the only shipped behavior until the provider exists.
- Do not expose a SIGMA CVar or local fallback mode before provider selection can satisfy the required resources.
- Add a real provider boundary before exposing SIGMA as a supported mode:
  - `Renderer/Private/Denoising/Nrd/NrdProvider.*`
  - `Renderer/Private/Denoising/Nrd/NrdSigmaShadowPass.*`
  - native interop through RHI/provider services
  - NRD common settings, denoiser settings, resource snapshot, permanent/transient pool ownership

Reference mapping:

- NRD states that the application creates resources and dispatches compute work from NRD descriptors; NRD itself does not allocate or issue GAPI calls.

### P0.3 Create A Reconstruction Provider Contract Separate From Upscaling

Current problem:

- `UpscalerInputContract` is correct for DLSS-SR/DLAA, but DLRR needs a wider denoising/reconstruction input set.
- `RendererProviderModel` has `Upscaler`, `Denoiser`, and `FrameGeneration`; it has no first-class `RayReconstruction` category.
- `FrameAssemblyDenoiserProviderResources.IndirectReconstruction` exists, but provider selection cannot reject missing DLRR resources because no reconstruction contract exists.
- `StreamlineDlssRuntime` loads and evaluates `sl::kFeatureDLSS`, not `sl::kFeatureDLSS_RR`, and tags only DLSS-SR resources.

Decision:

- Add a provider-neutral `RayReconstructionInputContract` rather than extending `UpscalerInputContract`.
- Add provider category `RayReconstruction` or a more precise `DenoisingReconstruction` category. Do not overload `Upscaler`.
- Build DLRR on top of Streamline because DLRR is evaluated through Streamline, but keep the Sparkle-side contract separate:
  - `ScalingInputColor` / noisy HDR color
  - output color
  - diffuse albedo
  - specular albedo
  - normal and roughness, packed or separate by explicit mode
  - depth with linear/HW metadata
  - motion vectors and scale
  - specular hit distance or specular motion vectors
  - exposure and reset state
  - render/output extent and frame token metadata

Reference mapping:

- Streamline DLRR requires the above tags and common constants. It can share DLSS quality mode, but it is not the same resource contract as DLSS-SR.

### P0.4 Reference Path Must Write Its Own Provider Guides

Current problem:

- Sparkle's reference path traces primary rays through TLAS.
- GBuffer normals/depth/motion are realtime-deferred products and are invalid in reference mode.
- Any future DLRR/NRD path for reference rendering cannot borrow GBuffer products unless an explicit reference visibility/guide pass writes equivalent products.

Decision:

- Treat reference reconstruction as path-tracer-owned guide generation.
- If reference mode wants denoising/reconstruction, `ReferencePathTracing` or a sibling reference-guide pass must output:
  - primary hit depth/viewZ
  - shading normal
  - roughness
  - diffuse/specular albedo
  - hit distance
  - motion vectors if temporal reconstruction is enabled
- Do not make shared post/provider code silently pull realtime GBuffer handles in reference mode.

Reference mapping:

- RTXPT pure path tracing generates guide buffers from the path tracer.
- Falcor's path tracer may consume lightweight primary visibility, but path tracing remains the owner of path generation and path outputs.

## P1 Decisions

### P1.1 Replace Device-Depth Ambiguity With Provider-Specific Depth Products

Current problem:

- `UpscalerInputContract` labels depth as `ReversedDeviceDepth`.
- DLRR may consume HW depth or linear depth, but the tag and metadata must be exact.
- NRD uses viewZ-style guide semantics in its core documentation.

Decision:

- Keep `SceneDepth` / `GBufferDeviceZ` as engine products.
- Add provider conversion passes only when needed:
  - `ProviderLinearViewZ`
  - `ProviderNormalRoughness`
  - optional `ProviderSpecularHitDistance`
- Provider builders select the exact product they require. They do not reinterpret a device-depth handle ad hoc.

### P1.2 Stop Treating Guide Buffers As Debug Side Products

Current problem:

- Reconstruction guide buffers are currently additional lighting targets.
- They are useful, but the ownership model is still "lighting target allocation grew more fields."

Decision:

- Keep guide writers close to path sampling and indirect lighting, but publish them through a provider guide product struct.
- Do not add a global signal registry. Do add clear product ownership:
  - writer: indirect/reference/path pass
  - semantic owner: reconstruction provider contract
  - consumer: NRD/DLRR/debug capture

### P1.3 Treat NRD REBLUR/RELAX And DLRR As Different Consumers

Current problem:

- The signal table maps both NRD and Streamline concepts, but the code has no consumer-specific contract.

Decision:

- `RayReconstructionInputContract` should describe canonical Sparkle products.
- Provider adapters map canonical products to NRD or Streamline tags/settings.
- Avoid a lowest-common-denominator layout that weakens both integrations.

Reference mapping:

- NRD expects demodulated signals and guide data; Streamline DLRR expects tagged noisy color and specific material/geometry guide resources.

## P2 Decisions

### P2.1 Keep `FrameRenderPath`, But Do Not Grow It Into A God Mode Switch

Keep:

- `FrameRenderPath::RealtimeDeferred`
- `FrameRenderPath::PathTracedReference`
- build-result reporting of the chosen path

Add later only if needed:

- `PathTracedRealtime`
- `RealtimeDeferredWithReferenceCompare`

Do not add:

- a large `AddRealtimePath()` wrapper
- a mode manager that hides the current visible hierarchy in `Frame.cpp`

### P2.2 Move Provider History Ownership Out Of `FramePipeline` When Providers Become Real

Current state:

- Exposure history and shadow denoise history are in `FramePipeline`.
- That is acceptable while SIGMA is not a real provider.

Decision:

- When NRD lands, provider-owned permanent pools should move into the provider runtime or a small provider history owner.
- `FramePipeline` should reset and bind high-level history state, not know every provider texture pool.

Reference mapping:

- NRD distinguishes permanent and transient resource pools owned by the app/provider integration.

## Proposed Stage Additions

### Stage 6R: Rebuild Direct Shadows For SIGMA

Goal:

- Make direct shadows denoiser-ready before implementing NRD, with the editor still showing the current raw-shadow result.

Required changes:

- Split direct shadow signal generation from direct-light radiance accumulation.
- Replace aggregate per-pixel raw shadow signal with a provider-compatible light selection or per-light product.
- Direct lighting consumes `ShadowVisibilitySource::Raw` or `ShadowVisibilitySource::Denoised`.
- Keep physical light units and area-light sampling unchanged.
- Add the raw/denoised visibility source as a renderer setting, but expose only raw until Stage 6N provides NRD.

Editor result:

- The viewport still matches the current raw-shadow look.
- The direct-light pass no longer owns denoiser input generation, so Stage 6N can visibly change shadows without rewriting lighting.

References:

- NRD SIGMA shadow-only denoiser.
- RTXPT/Falcor explicit render-graph products for denoiser-facing signals.

### Stage 6N: Add NRD Provider Runtime

Goal:

- Implement a provider boundary capable of executing SIGMA without making NRD mandatory, and make the result selectable in the editor.

Required changes:

- Add `NrdProvider` with capability query, initialization, resize/recreate, history pool ownership, common settings, and dispatch execution.
- Consume frame-graph resources through native RHI interop.
- Recreate provider resources on resize and render-path changes.
- No local blur fallback.
- Add renderer/editor settings for SIGMA mode, history reset, and the minimal quality knobs required by NRD.
- Keep the UI disabled or hidden when the NRD provider is not available.

Editor result:

- A one-sample soft-shadow scene can toggle raw visibility versus NRD SIGMA denoised visibility.
- Shadow penumbra noise decreases while unoccluded direct-light intensity remains unchanged.

References:

- NRD Integration / ResourceSnapshot / permanent and transient pool model.

### Stage 11R: Add Ray Reconstruction Provider Contract And Guides

Goal:

- Make DLRR and future reconstruction providers selectable only when all required resources exist, and write the guide buffers needed for a visible DLRR pass.

Required changes:

- Add `RayReconstructionInputContract`.
- Add `RendererProviderCategory::RayReconstruction`.
- Convert current `FrameAssemblyDenoiserProviderResources.IndirectReconstruction` into explicit provider input products.
- Add contract validation for DLRR-required inputs and formats.
- Write provider-ready noisy HDR color, diffuse albedo, specular albedo, normal/roughness, depth, motion, exposure, and specular hit-distance or specular motion-vector products from the realtime path.
- Keep guide writers near path/lighting code; do not build a global signal registry or duplicate material packing helpers.

Editor result:

- Reconstruction mode remains unavailable until all required resources exist.
- The editor can show an unreconstructed noisy indirect/specular scene using the same lighting math that DLRR will consume.

References:

- Streamline DLRR resource tags.
- AMD noisy color plus guide-buffer reconstruction pattern.

### Stage 11D: Implement Streamline DLRR Provider

Goal:

- Implement DLRR through Streamline without merging it into DLSS-SR code, and expose it as a visible editor reconstruction mode.

Required changes:

- Query and load `sl::kFeatureDLSS_RR`.
- Set DLRR options and common constants.
- Tag DLRR resources: noisy HDR color, output color, albedo, specular albedo, normal/roughness, depth, motion vectors, specular hit distance or specular motion vectors, exposure/reset.
- Restore command state after `slEvaluateFeature`.
- Reinitialize or reject unsupported dynamic-resolution changes per Streamline guidance.
- Keep DLRR provider code separate from the DLSS-SR provider, sharing only Streamline runtime primitives that own real API policy.
- Add renderer/editor settings for reconstruction off/DLRR and any required Streamline quality mode.

Editor result:

- A noisy indirect/specular scene can toggle unreconstructed output versus DLRR output.
- The viewport output remains explicit: final linear HDR lighting goes through the same exposure and presentation path after reconstruction.

References:

- Streamline DLSS-RR Programming Guide.

### Stage 12G: Reference Path Guide Outputs

Goal:

- Make path-traced reference outputs comparable to realtime and provider-ready without borrowing GBuffer products.

Required changes:

- Add optional reference guide targets for primary depth/viewZ, normal, roughness, albedo/F0, first-hit distance, and path lobe metadata.
- Keep them under `Frame/Reference`.
- Let reference denoising/reconstruction consume those guides explicitly.

References:

- RTXPT pure path tracer guide-buffer generation.
- Falcor PathTracer render graph ownership.

## What To Delete Or Avoid

- Delete any fake SIGMA fallback once provider selection exists. Raw visibility is the fallback.
- Do not keep aggregate shadow signals as the long-term SIGMA input.
- Do not extend `UpscalerInputContract` into a DLRR kitchen sink.
- Do not add backend-specific shaders for provider work. Backend-specific interop belongs in provider runtime/RHI bridges.
- Do not make reference mode publish realtime GBuffer products unless a reference guide pass actually writes them.
- Do not add wrappers that only rename a comparison or forward a struct.

## Suggested Priority Order

1. Stage 6R: split direct shadow signal generation from direct lighting while preserving the current raw-shadow viewport result.
2. Stage 6N: add NRD provider runtime and editor-visible SIGMA shadow denoising.
3. Stage 11R: add `RayReconstructionInputContract`, provider category, and realtime guide buffers needed by DLRR.
4. Stage 11D: add Streamline DLRR provider and editor-visible ray reconstruction.
5. Stage 12G: make reference path write its own guides after realtime SIGMA/DLRR results are visible.
6. Final cleanup: collapse stale shadow/reconstruction docs and remove obsolete "future" wording after provider integrations land.

## Bottom Line

Sparkle has the right top-level path split now. The next quality jump is editor-visible SIGMA and DLRR, not another broad frame orchestrator refactor. SIGMA needs shadow visibility before lighting and a real NRD provider. DLRR needs reconstruction guide buffers and a Streamline DLRR provider, not an upscaler contract stretched sideways. Reference path tracing still needs its own guides, but that comes after the realtime editor results are visible.
