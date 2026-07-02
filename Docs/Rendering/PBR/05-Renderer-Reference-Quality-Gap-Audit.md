# Sparkle Renderer Reference Quality Gap Audit

This audit compares the current Sparkle renderer architecture against NVIDIA NRD SIGMA, Streamline DLSS Ray Reconstruction, RTXPT, Falcor PathTracer, and AMD neural denoising/upscaling patterns. It is intentionally decision-oriented: if a current path blocks physical correctness or reference-quality integration, the recommendation is to replace it rather than preserve compatibility.

## Reference Lineage

- NVIDIA NRD: API-agnostic spatio-temporal denoisers for low-spp signals, using GBuffer guides such as normal, roughness, viewZ, and motion vector. SIGMA is the shadow-only denoiser. NRD does not allocate resources; the application owns texture creation and dispatches the compute passes from NRD descriptors. <https://github.com/NVIDIA-RTX/NRD>
- NVIDIA RTXDI: ReSTIR DI / ReGIR / ReSTIR GI provide reference many-light sampling structure. The application owns light buffers, GBuffer/material/ray-tracing bridge code, resource allocation, and final shading; RTXDI provides light sampling/resampling math and reservoir data flow. <https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md>
- NVIDIA Streamline DLSS Ray Reconstruction: DLRR requires provider-tagged noisy HDR color plus guide buffers such as diffuse albedo, specular albedo, normals, roughness, motion vectors, depth, and either specular motion vectors or specular hit distance. It is an extension around DLSS evaluation, but its resource contract is much broader than DLSS Super Resolution. <https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md>
- NVIDIA RTXPT: pure path tracer, no raster dependency in the main configuration, with reference and real-time modes, path-space layer decomposition, guide-buffer generation, NRD, and Streamline integration. <https://github.com/NVIDIA-RTX/RTXPT>
- NVIDIA Falcor PathTracer: separate render pass/render graph. It can use lightweight primary visibility input, then owns path generation, path tracing, direct-light sampling at path vertices, accumulation, and optional denoiser graph wiring. <https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md>
- AMD FidelityFX Shadow Denoiser: useful reference for explicit shadow-mask staging, bitmask packing, tile classification, temporal reprojection, and edge-aware filtering, but documented as a single-light shadow-mask denoiser. Sparkle must not treat it as a many-light direct-shadow solution without an explicit per-light or sampled-light policy. <https://gpuopen.com/manuals/fidelityfx_sdk/techniques/denoiser/>
- AMD neural supersampling/denoising: real-time path tracing reconstruction benefits from noisy color plus guide buffers such as albedo, normal, roughness, depth, and specular hit distance, with denoising and upscaling treated as explicit reconstruction stages. <https://gpuopen.com/learn/neural_supersampling_and_denoising_for_real-time_path_tracing/>

## Executive Verdict

Sparkle is now structurally pointed in the right direction: `FrameRenderPath` separates realtime deferred rendering from path-traced reference rendering, reference targets live in `Frame/Reference`, linear HDR is preserved into presentation, and reconstruction resources are not hidden inside the DLSS-SR upscaler contract.

The renderer is not yet at NVIDIA-reference integration quality. The largest blockers are:

1. Stage 6M now removes first-light shadowing and uses an RTXDI-shaped initial sampled-light product, but there is no temporal/spatial reservoir reuse yet and no NRD provider execution.
2. SIGMA is still not a provider path. Raw sampled-light visibility exists before direct lighting, but no NRD provider consumes it, owns history, or writes denoised visibility.
3. DLRR is not a provider path yet; the existing Streamline runtime only evaluates DLSS Super Resolution / DLAA.
4. Reconstruction guide buffers exist as frame assembly slots, but there is no provider-neutral reconstruction contract or Streamline DLRR tag path.
5. Reference path tracing owns primary rays, but provider guide buffers are still mostly realtime-GBuffer-shaped. Reference mode must write its own guides if it wants denoising/reconstruction.

## First Target: Editor-Visible SIGMA And DLRR

The next work should produce visible editor results before broad cleanup continues. The first target is not a perfect final provider architecture; it is a clean, reference-shaped vertical slice that can be switched on in the editor and judged visually.

Priority:

1. Finish SIGMA shadow denoising end-to-end on top of the Stage 6M sampled-light visibility product: NRD provider execution, persistent provider history, editor setting, and direct lighting consuming raw or denoised visibility.
2. Add temporal/spatial reservoir reuse or an RTXDI provider boundary after the initial sampled-light contract is stable.
3. Finish DLRR/ray reconstruction end-to-end: provider-neutral reconstruction contract, guide buffers, Streamline DLRR resource tags, editor setting, and reconstructed output visible in the viewport.
4. After those are visible, return to reference-path guide outputs and deeper renderer architecture cleanup.

Editor-visible definition of done:

- A renderer settings panel exposes shadow visibility mode as raw or NRD SIGMA only when an NRD provider is available.
- A noisy low-sample shadow scene visibly changes between raw visibility and SIGMA denoised visibility without changing light energy.
- A renderer settings panel exposes reconstruction mode as off or Streamline DLRR only when the Streamline provider can satisfy the required tags.
- A noisy indirect/specular reconstruction scene visibly changes between unreconstructed and DLRR output while preserving linear HDR lighting before presentation.
- Unsupported provider modes are not shown as working features; they remain absent or disabled with a short reason at the provider boundary.

## Universal Implementation Prompt Rules

Every staged prompt below is ready to use as an implementation request. Each stage must leave the codebase simpler than it found it.

Prompt guardrail for every stage:

- Include reference lineage and reuse/DRY audit in the implementation note.
- Scan existing bodies before adding or moving logic.
- Start with a code denoising pass in the touched area: delete stale scaffolding, no-op code, unused settings, fake provider paths, needless logging, duplicate docs, and wrappers that only rename or forward parameters.
- Keep a helper only when it owns real policy, math, IO binding, resource lifetime, repeated behavior, or a meaningful architecture boundary.
- Follow NVIDIA/AMD reference patterns concretely enough that the shape is recognizable:
  - NRD: app/provider owns resources, permanent/transient pools, dispatch from NRD descriptors, and explicit guide inputs.
  - Streamline DLRR: tagged resources, common constants, feature availability, command-state restoration, and provider-boundary failure.
  - RTXPT/Falcor: explicit render-graph products for path/shadow/reconstruction signals, path-owned guide buffers, and separate reference/realtime paths.
  - AMD/FidelityFX style: explicit reconstruction stage inputs, temporal resource ownership, and no hidden coupling between upscaling and denoising.
- Avoid adding backend-specific shaders or renderer-facing backend branches. Backend/native details belong in RHI/provider bridges.
- Preserve current visible behavior unless the stage explicitly changes it and names the editor-visible result.
- If a stage needs a new layer, justify why existing homes cannot own the behavior.

## Current Source-Backed State

| Area | Current source | Assessment |
| --- | --- | --- |
| Render path selection | `FrameRenderPath`, `BuildFrame`, `FrameBuildResult`, `FrameGraphBuildResult`, `FramePipeline` | Good direction. The built graph now reports the path it actually contains. |
| Reference path ownership | `Frame/Reference/*`, `Passes/Reference/ReferencePathTracingPass.*`, `Passes/Reference/ReferencePathTracing.hlsl` | Good direction. Reference targets are no longer lighting targets. |
| Shadow visibility signal | `Frame/Lighting/ShadowVisibility.cpp`, `Frame/Lighting/Shadows/DirectShadowSignal.cpp`, `DirectShadowSignal.hlsl`, `RayTracedShadowDenoiserInputs.hlsli` | Stage 6M writes raw visibility/hit-distance before direct lighting and pairs it with `DirectShadowLightSample` identity/PDF. No SIGMA mode is exposed until a real provider exists. |
| Direct shadows | `DirectLighting.hlsl`, `DirectLightingPass.*`, `DirectShadowSignalPass.*` | Direct lighting is now backend-agnostic and consumes the sampled-light plus visibility products. It no longer traces shadows inline or mixes denoised-first-light with raw-rest paths. |
| Many-light shadow sampling | `DirectLightSampling.hlsli`, `LightSampling.hlsli`, `AreaLights.hlsli` | Initial Stage 6M slice samples one contribution-weighted direct-light candidate per pixel across directional, point, spot, and rect lights. Next NVIDIA-quality step is reservoir reuse / RTXDI provider integration. |
| Shadow history | Not allocated in the current runtime | Correct current state. Future SIGMA history must be introduced by the provider pass that consumes it. |
| Upscaler provider | `UpscalerInputContract`, `NvidiaDlssUpscalerProvider`, `StreamlineDlssRuntime` | DLSS-SR/DLAA shaped. This should stay separate from DLRR. |
| Reconstruction resources | `FrameAssemblyDenoiserProviderResources`, `FrameProviderInputs`, `LightingRenderTargets` guide slots | Good vocabulary, but not a complete provider contract and not tagged/evaluated by any provider. |
| Physical lighting contract | `SurfaceLighting`, `AreaLights`, `PathSampling`, `PathLighting`, lighting target contract docs | Much improved. Main remaining risk is denoiser/reconstruction signal correctness, not basic radiance units. |

## P0 Decisions

These are required before claiming SIGMA or DLRR-quality integration.

### P0.1 Split Direct Visibility From Direct Lighting Accumulation

Current problem:

- Before Stage 6R, `DirectLighting.hlsl` traced a direct-light sample, immediately multiplied lighting by `shadow.Visibility`, and also wrote `ShadowVisibilitySignalTexture`.
- A denoiser can only improve a signal that is consumed after denoising. Direct lighting now consumes the Stage 6M sampled-light visibility product written by `DirectShadowSignal`.
- The old raw shadow output was an aggregate "most occluding" per-pixel signal across all direct light samples. Stage 6R removed that aggregate, and Stage 6M removed the selected-first-light shortcut by pairing visibility with direct-light sample identity/PDF.

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

### P0.1A Replace Selected-Light Shadowing With Many-Light Visibility

Resolved Stage 6M problem:

- `DirectShadowSelection::SelectFirstShadowCastingLight` chose the first shadow-casting light by type order.
- That shortcut was acceptable only as a Stage 6R proof that raw visibility can be generated before direct lighting. It was not physically correct, did not scale to multiple soft-shadowing lights, and could not produce editor-visible denoising for all direct shadows.
- Denoising a single selected light while tracing every other light raw created a misleading split: only one light could become smooth, and quality depended on light ordering rather than radiance, importance, or visibility.
- AMD FidelityFX Shadow Denoiser is explicitly shaped around a single-light shadow mask. It can inform per-light staging, tile classification, packing, and temporal filtering, but it is not a complete many-light solution by itself.
- NVIDIA's many-light reference path is RTXDI/ReSTIR-style: collect all light data, sample/resample important light candidates per pixel, trace visibility for the selected reservoir sample, shade using the chosen sample and PDF/weight, and optionally feed confidence/denoiser inputs.

Implemented decision:

- Stage 6M deletes `SelectFirstShadowCastingLight` and replaces it with an RTXDI-style initial sampled-light product.
- `DirectLightSampling.hlsli` samples one contribution-weighted candidate per pixel across all represented directional, point, spot, and rect lights, stores light type/index plus selection PDF in `DirectShadowLightSample`, and pairs it with raw visibility/hit-distance in `ShadowVisibilitySignalRaw`.
- `DirectLighting.hlsl` shades from the sampled-light product and the paired visibility signal. It no longer loops over all lights while tracing non-selected lights raw, and it no longer has ray-query/no-ray/device-address variants.
- Do not reintroduce an implicit "first light wins" policy, a global aggregate shadow mask, or a denoised-first-light plus raw-rest hybrid.

Remaining NVIDIA-quality work:

- Add temporal/spatial reservoir reuse or integrate an RTXDI-style provider so the initial sampled-light product becomes a stable many-light direct-lighting solution rather than a one-sample vertical slice.
- Stage 6N must make NRD SIGMA consume and produce visibility for the same sampled-light identity; it must not denoise an unordered aggregate.

Reference mapping:

- RTXDI integrates deeply with the renderer, with app-owned light buffers, GBuffer/material/ray-tracing bridge code, reservoir buffers, light sampling/resampling passes, final visibility ray, optional confidence inputs, and denoising/composition after sampling.
- NRD SIGMA is per-light shadow denoising. It can denoise a chosen direct-light sample or an explicit per-light shadow mask, but it must not be fed an unordered aggregate that no longer maps to a light.
- AMD FidelityFX Shadow Denoiser confirms the single-light shadow-mask model. For Sparkle, that makes it a per-light/single-sample building block, not the solution for all lights.

### P0.2 Do Not Claim SIGMA Until There Is A Real NRD Provider

Current problem:

- Sparkle has no linked NRD provider execution path.
- The previous local SIGMA CVar/contract/history scaffold did not execute a provider and has been removed.
- The current raw shadow output is sampled-light shaped. It is enough to prove the render-graph staging, but not enough to claim SIGMA until Stage 6N adds a real NRD provider and history ownership.

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

## Ready Implementation Prompts

### Stage 6R: Rebuild Direct Shadows For SIGMA

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Before writing new code, do a code denoising pass over direct-light/shadow scheduling and delete or collapse anything that only exists for fake denoising, duplicated visibility ownership, stale settings, or pass-local shadow policy. Do not add wrappers that only rename `raw` versus `denoised`; keep a helper only if it owns real light selection, signal packing, resource declaration, or visibility-source policy.

Goal:

- Make direct shadows denoiser-ready before implementing NRD, with the editor still showing the current raw-shadow result.

Reference lineage:

- NVIDIA NRD SIGMA: raw shadow signal plus guide inputs must exist before denoising; denoised visibility is consumed after provider execution.
- RTXPT/Falcor: denoiser-facing visibility products are explicit render-graph products, not incidental side effects of final radiance accumulation.
- AMD/FidelityFX-style reconstruction staging: noisy signal generation and reconstruction/denoising are separate stages with explicit resources.

Existing bodies to scan first:

- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/RayTracing/Shadows/*`
- `Engine/Renderer/Private/Frame/Lighting/*`
- `Engine/Renderer/Private/Passes/Deferred/DirectLightingPass.*`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/*`
- `Engine/Renderer/Private/Frame/Core/FrameAssembly.h`

Required implementation:

- Split direct shadow signal generation from direct-light radiance accumulation.
- Replace aggregate per-pixel raw shadow signal with a provider-compatible selected-light or per-light product, with the exact policy named in code and notes. A selected-light product is Stage 6R scaffolding only and must be removed by Stage 6M.
- Direct lighting consumes an explicit visibility source: raw in this stage, denoised later in Stage 6N.
- Keep physical light units and area-light sampling unchanged.
- Add the raw/denoised visibility source as a renderer setting only if it is not misleading; expose raw only until Stage 6N provides NRD.
- Keep shader module direction intact: light/BRDF evaluation stays in `Lighting/*`, shadow tracing stays in `RayTracing/Shadows/*`, pass scheduling stays in frame/pass code.

Editor result:

- The viewport matches the current raw-shadow look.
- The direct-light pass no longer owns denoiser input generation, so Stage 6N can visibly change shadows without rewriting lighting.

Acceptance criteria:

- Code denoising gate: touched direct-light/shadow code removes stale wrappers, no-op provider hooks, duplicate signal declarations, and needless settings before adding new logic.
- Raw direct shadows render the same as before within expected sampling variance.
- Shadow visibility signal generation is its own pass/resource path and does not apply final BRDF radiance.
- Direct lighting consumes a visibility resource instead of tracing/applying shadow visibility internally.
- The raw visibility signal is provider-shaped enough for SIGMA integration or explicitly documents the remaining blocker.
- No backend-specific shader/pass names are added.
- Reference compliance note maps the final structure to NRD SIGMA and RTXPT/Falcor render-graph product separation.
- Reuse/DRY note lists the existing bodies scanned and explains why any new helper earns its place.

Stage 6R implementation note:

- Superseded by Stage 6M. Stage 6R proved the render-graph staging shape, but its selected-light shortcut and direct-light raw-rest hybrid are no longer acceptable current architecture.
- Reference compliance retained: `DirectShadowSignal` writes raw visibility/hit distance before direct lighting, and `DirectLighting` reads that signal instead of writing denoiser input as a side effect.
- Reuse/DRY audit: scanned `DirectLighting.hlsl`, `RayTracing/Shadows/*`, `Frame/Lighting/*`, `DirectLightingPass.*`, `DirectShadowSignalPass.*`, `RayTracedShadowPassData`, and `FrameAssembly.h`. Kept existing area-light policy, alpha-tested shadow tracing, and frame-graph resource ownership. Stage 6M deleted `DirectShadowSelection.hlsli` and kept signal packing beside `RayTracedShadowDenoiserInputs` because packing is the real signal boundary.

### Stage 6M: Replace Selected-Light Shadows With Many-Light Sampling

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Begin by deleting or collapsing the Stage 6R selected-light shortcut. Do not keep `SelectFirstShadowCastingLight`, light-type-order priority, hidden first-light policy, aggregate "most occluding" masks, or a denoised-first-light/raw-rest hybrid. Do not add wrappers that only rename light samples, reservoirs, or visibility products; keep a helper only if it owns real light indexing, candidate sampling, reservoir math, per-light product allocation, resource declaration, or visibility-source policy.

Goal:

- Support soft shadows from all shadow-casting direct lights through a reference-shaped many-light visibility pipeline, so SIGMA/denoising can improve the full direct-light result rather than one arbitrary light.

Reference lineage:

- NVIDIA RTXDI/ReSTIR DI: direct illumination over many dynamic lights is solved by app-owned light buffers, light sampling/resampling, reservoir products, final selected-light visibility rays, optional confidence inputs, and denoising/composition after sampling.
- NVIDIA NRD SIGMA: SIGMA is a per-light shadow-only denoiser. The denoised signal must map to a known light/sample, not an unordered aggregate.
- NVIDIA RTXPT/Falcor: direct-light sampling is an explicit path/render-graph product, and denoiser-facing products are not side effects of final radiance accumulation.
- AMD FidelityFX Shadow Denoiser: shadow denoiser staging, bitmask packing, tile classification, temporal reprojection, and edge-aware filtering are useful references, but the documented shadow mask is single-light. Use it as a per-light/single-sample staging model, not as a many-light aggregation model.

Existing bodies to scan first:

- `Engine/Assets/Shaders/RayTracing/Shadows/DirectShadowSelection.hlsli`
- `Engine/Assets/Shaders/Lighting/LightSampling.hlsli`
- `Engine/Assets/Shaders/Lighting/AreaLights.hlsli`
- `Engine/Assets/Shaders/Lighting/PunctualLights.hlsli`
- `Engine/Assets/Shaders/Passes/Deferred/DirectShadowSignal.hlsl`
- `Engine/Assets/Shaders/Passes/Deferred/DirectLighting.hlsl`
- `Engine/Assets/Shaders/RayTracing/Shadows/*`
- `Engine/Renderer/Private/Frame/Lighting/Shadows/*`
- `Engine/Renderer/Private/Passes/Deferred/DirectShadowSignalPass.*`
- `Engine/Renderer/Private/Passes/Deferred/DirectLightingPass.*`
- `Engine/GameFramework/Private/Scene/Lighting/Snapshots/*`
- `Engine/RHI/Public/Resources/RenderViewLightingData.h`

Required implementation:

- Delete `SelectFirstShadowCastingLight` and replace it with one explicit many-light policy.
- Preferred policy: implement an RTXDI/ReSTIR-shaped sampled-light direct-shadow path:
  - Build or expose a compact direct-light table over all shadow-casting directional, point, spot, rect, and future emissive/area lights.
  - Generate initial light candidates using power/solid-angle/BRDF-aware importance where available.
  - Store a per-pixel selected light sample/reservoir product containing light type/index, sample position/direction, PDF or inverse PDF/weight, confidence, and any data needed for final BRDF evaluation.
  - Trace visibility for that selected sample and write raw visibility/hit distance tied to the same sample identity.
  - Direct lighting shades from the sampled-light product and its PDF/weight; it does not loop all lights and secretly raw-trace unrepresented lights in the denoised mode.
  - Add temporal/spatial reservoir reuse only after the initial sample/visibility/shading contract is correct.
- Fallback policy for small scenes only: implement explicit per-light or per-light-tile visibility products:
  - Each denoised visibility result must retain light identity.
  - Resource count, format, max light count, tiling policy, and editor limits must be explicit.
  - The path must not collapse multiple lights into a single visibility value before denoising.
- Keep physical light units and area-light sampling unchanged.
- Keep low-level shadow tracing in `RayTracing/Shadows/*`; keep light sampling in `Lighting/*`; keep pass scheduling in frame/pass code.
- If the first vertical slice only supports punctual/rect lights, document unsupported light classes as absent from the sampled-light table rather than silently raw-tracing them.

Editor result:

- A scene with multiple shadow-casting lights shows all represented lights using the same shadow-sampling/visibility-source policy.
- Toggling denoising later in Stage 6N can visibly smooth all represented soft shadows, not only the first shadow-casting light in a fixed type order.

Acceptance criteria:

- Code denoising gate: `DirectShadowSelection::SelectFirstShadowCastingLight` and any first-light-priority policy are removed.
- No final shader/pass code contains a "first shadow-casting light" selection path.
- All represented direct lights participate in one explicit sampled-light or per-light visibility policy.
- Direct lighting no longer combines one denoised selected light with raw-traced non-selected lights in the same denoised mode.
- The raw visibility signal carries enough light/sample identity for the denoiser and final lighting consumer to agree on what was filtered.
- The chosen policy has a clear cost model: RTXDI-style one/few samples per pixel with reservoir weights, or explicit per-light/tiled products with limits.
- Area-light soft shadows remain physically sampled; changing light size changes penumbra through sampling, not through denoiser hacks.
- No backend-specific shader/pass names are added.
- Reference compliance note maps light table/candidate generation/reservoir or per-light product, visibility tracing, confidence, and denoise/composite order to RTXDI/NRD/RTXPT/Falcor/AMD.
- Reuse/DRY note lists the existing light sampling, area-light, shadow tracing, frame-graph, and scene-light snapshot bodies scanned, and explains why any new helper earns its place.

Stage 6M implementation note:

- Implemented policy: initial RTXDI/ReSTIR-shaped sampled-light slice. `DirectLightSampling.hlsli` samples one contribution-weighted direct-light candidate per pixel across directional, point, spot, and rect lights, stores light type/index plus selection PDF in `DirectShadowLightSample`, and `DirectShadowSignal` writes paired raw visibility/hit-distance in `ShadowVisibilitySignalRaw`.
- Direct lighting now consumes the sampled-light and visibility products only. It no longer traces direct shadows inline, no longer combines one selected denoiser signal with raw-traced non-selected lights, and no longer has direct-lighting ray-query/no-ray/device-address shader variants.
- Reference compliance: the structure follows RTXDI's initial candidate/sample plus final visibility ray shape, keeps NRD/SIGMA-compatible visibility before lighting, and keeps AMD FidelityFX-style shadow-mask staging as a per-sample/per-light building block rather than a many-light aggregate. Local deviation: Sparkle has no temporal/spatial reservoir reuse or RTXDI provider yet, so the current estimator is a one-sample vertical slice.
- Reuse/DRY audit: scanned `DirectShadowSelection.hlsli`, `LightSampling.hlsli`, `AreaLights.hlsli`, `PunctualLights.hlsli`, `DirectShadowSignal.hlsl`, `DirectLighting.hlsl`, `RayTracing/Shadows/*`, `Frame/Lighting/Shadows/*`, `DirectShadowSignalPass.*`, `DirectLightingPass.*`, scene light snapshot code, and `RenderViewLightingData.h`. Deleted `DirectShadowSelection.hlsli` and direct-lighting backend wrapper shaders. Kept `DirectLightSampling.hlsli` because it owns real light indexing, candidate weighting, sample identity packing, and selection PDF policy.

### Stage 6N: Add NRD SIGMA Provider Runtime

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Begin by verifying Stage 6M removed selected-light ordering and produced a many-light sampled-light or per-light visibility contract. Do not add a fake blur fallback, local approximation, provider-looking wrapper without NRD execution, or a renderer-global history manager. Provider resources and history must be owned by the NRD provider boundary, not scattered through frame orchestration.

Goal:

- Implement a provider boundary capable of executing SIGMA without making NRD mandatory, and make the result selectable in the editor.

Reference lineage:

- NVIDIA NRD: application creates resources, owns permanent/transient pools, translates NRD descriptors into graphics API dispatches, and feeds normal/depth/motion/history guides.
- RTXPT/Falcor: denoiser execution is a provider/render-graph stage between noisy signal generation and final lighting use.
- AMD/FidelityFX-style temporal ownership: provider history resets and resource recreation are explicit on resize, camera cut, render-path change, and feature toggles.

Existing bodies to scan first:

- Stage 6R shadow visibility resources and passes.
- `Engine/Renderer/Private/Providers/*`
- `Engine/Renderer/Private/FramePipeline/*`
- `Engine/Renderer/Private/FrameGraph/*`
- `Engine/Renderer/Private/RayTracing/Effects/Shadows/*`
- RHI native interop surfaces and resource-state helpers.
- Editor rendering settings panel and renderer settings storage.

Required implementation:

- Add `NrdProvider` with capability query, initialization, resize/recreate, shutdown, provider status, and version/availability reporting.
- Add SIGMA execution using NRD descriptors and app-owned permanent/transient resource pools.
- Consume Stage 6M raw shadow visibility plus light/sample identity, normal, depth/viewZ, motion, jitter/history metadata, and reset state through explicit frame-graph resources.
- Output denoised visibility as a resource consumed by direct lighting.
- Recreate provider resources on resize and render-path changes.
- Reset provider history on camera cut, resize, temporal invalidation, render-path change, and feature toggle.
- Add renderer/editor settings for raw versus NRD SIGMA, history reset, and only the minimal NRD quality knobs needed for the first visible result.
- Disable or hide SIGMA in the editor when provider capability is unavailable.

Editor result:

- A one-sample soft-shadow scene can toggle raw visibility versus NRD SIGMA denoised visibility.
- Shadow penumbra noise decreases while unoccluded direct-light intensity remains unchanged.

Acceptance criteria:

- Code denoising gate: no fake provider path, no fallback blur, no dormant CVar, no unused history allocation, and no wrapper that only forwards NRD settings.
- Stage 6M many-light visibility is complete; SIGMA is not exposed on top of `SelectFirstShadowCastingLight` or an equivalent first-light shortcut.
- `SIGMA unavailable` is represented at the provider boundary, not by pretending raw visibility is SIGMA.
- Direct lighting can consume raw or denoised visibility through the same visibility-source boundary.
- NRD permanent/transient resources are owned by the provider runtime or a provider-local owner, not by broad `FramePipeline` fields.
- RHI/native interop is provider-scoped and does not leak D3D12/Vulkan details into general renderer passes.
- Editor settings expose only modes that can actually run.
- Reference compliance note maps resource ownership, dispatch flow, history reset, and guide inputs to NRD and RTXPT/Falcor patterns.
- Reuse/DRY note lists which frame-graph/provider/RHI bodies were reused instead of creating a parallel denoiser system.

### Stage 11R: Add Ray Reconstruction Provider Contract And Guides

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Start by denoising the current upscaler/denoiser/reconstruction boundary: remove fields that pretend DLRR is an upscaler input, collapse duplicate guide descriptions, and avoid a global signal registry. Do not add wrappers that merely repack material values if existing material/GBuffer/path helpers already own the policy.

Goal:

- Make DLRR and future reconstruction providers selectable only when all required resources exist, and write the guide buffers needed for a visible DLRR pass.

Reference lineage:

- NVIDIA Streamline DLRR: provider requires tagged noisy HDR color, output, diffuse/specular albedo, normal/roughness, depth, motion, exposure/reset, and specular hit-distance or specular motion-vector resources.
- NVIDIA NRD: guide/radiance resources must name demodulated/noisy signal semantics and temporal metadata explicitly.
- RTXPT/Falcor: path/reconstruction guide buffers are explicit outputs owned by path/realtime rendering, not debug-only side products.
- AMD neural denoising/upscaling: noisy color plus guide buffers feed a distinct reconstruction stage.

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
- Add `RendererProviderCategory::RayReconstruction` or a more precise denoising/reconstruction category.
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
- Reference compliance note maps each guide resource to Streamline DLRR, NRD, RTXPT/Falcor, or AMD reconstruction expectations.
- Reuse/DRY note lists which material, GBuffer, path, provider, and frame-assembly bodies were reused.

### Stage 11D: Implement Streamline DLRR Provider

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Start with a code denoising pass through the existing Streamline DLSS runtime: share only primitives that own real Streamline API policy, and keep DLSS-SR/DLAA separate from DLRR resource contracts. Do not merge DLRR into `UpscalerInputContract` or add DLRR-specific shader code unless a provider requirement genuinely needs it.

Goal:

- Implement DLRR through Streamline without merging it into DLSS-SR code, and expose it as a visible editor reconstruction mode.

Reference lineage:

- NVIDIA Streamline DLRR: query/load `sl::kFeatureDLSS_RR`, set options/common constants, tag all required resources, call `slEvaluateFeature`, and restore command state.
- NVIDIA RTXPT/Falcor: reconstruction is a pass/provider stage consuming explicit noisy/guide resources.
- AMD/FidelityFX-style provider integration: provider unavailable or incomplete resources disable the mode rather than running a hidden fallback.

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
- Reinitialize or reject unsupported dynamic-resolution changes per Streamline guidance.
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

### Stage 12G: Reference Path Guide Outputs

Implementation prompt:

Prompt guardrail: apply the universal implementation prompt rules above. Start by denoising reference/realtime guide boundaries: remove any code or docs implying reference mode may borrow realtime GBuffer handles. Do not add wrappers that only mirror realtime guide structs unless the reference path truly writes equivalent data.

Goal:

- Make path-traced reference outputs comparable to realtime and provider-ready without borrowing GBuffer products.

Reference lineage:

- RTXPT: pure path tracing generates guide buffers from the path tracer rather than relying on raster GBuffer products.
- Falcor PathTracer: path tracing is a separate render pass/graph owner for path outputs and optional denoiser inputs.
- Streamline/NRD/AMD reconstruction patterns: provider guide resources must come from the renderer path that produced the noisy signal.

Existing bodies to scan first:

- `Engine/Renderer/Private/Frame/Reference/*`
- `Engine/Renderer/Private/Passes/Reference/*`
- `Engine/Assets/Shaders/Passes/Reference/*`
- `Engine/Assets/Shaders/RayTracing/Path*.hlsli`
- `Engine/Assets/Shaders/RayTracing/RayTracingMaterialHit.hlsli`
- `Engine/Renderer/Private/Frame/Core/FrameAssembly.h`
- Stage 11R reconstruction contract code.

Required implementation:

- Add optional reference guide targets for primary depth/viewZ, normal, roughness, albedo/F0, first-hit distance, and path lobe metadata.
- Keep guide targets, allocation, and pass scheduling under `Frame/Reference`.
- Let reference denoising/reconstruction consume those guides explicitly.
- Do not publish realtime GBuffer products from reference mode unless a reference guide pass actually writes equivalent products.
- Reuse shared material, BRDF, sky, light, alpha-test, and path-sampling code; do not copy reference-only versions of those policies.

Editor result:

- Reference mode can expose guide outputs or feed reconstruction without depending on realtime deferred GBuffer products.
- Realtime SIGMA/DLRR results remain the first target; reference guide work follows after those editor-visible results exist.

Acceptance criteria:

- Code denoising gate: reference path code contains no fake GBuffer handles, no duplicated material/light policy, and no guide wrappers that do not write actual guide data.
- Reference guide resources are owned by `Frame/Reference` and have one writer.
- Shared path/material/light modules remain shared between realtime and reference paths.
- Provider/reconstruction code selects reference guides only when the reference path produced them.
- Reference compliance note maps guide ownership to RTXPT/Falcor and provider guide expectations to Streamline/NRD/AMD.
- Reuse/DRY note lists reused shader modules and any intentional local deviations.

## What To Delete Or Avoid

- Delete any fake SIGMA fallback once provider selection exists. Raw visibility is the fallback.
- Do not keep aggregate shadow signals as the long-term SIGMA input.
- Do not extend `UpscalerInputContract` into a DLRR kitchen sink.
- Do not add backend-specific shaders for provider work. Backend-specific interop belongs in provider runtime/RHI bridges.
- Do not make reference mode publish realtime GBuffer products unless a reference guide pass actually writes them.
- Do not add wrappers that only rename a comparison or forward a struct.

## Suggested Priority Order

1. Stage 6N: add NRD provider runtime and editor-visible SIGMA shadow denoising on the Stage 6M sampled-light visibility product.
2. Add temporal/spatial reservoir reuse or an RTXDI provider boundary for more stable many-light sampling.
3. Stage 11R: add `RayReconstructionInputContract`, provider category, and realtime guide buffers needed by DLRR.
4. Stage 11D: add Streamline DLRR provider and editor-visible ray reconstruction.
5. Stage 12G: make reference path write its own guides after realtime SIGMA/DLRR results are visible.
6. Final cleanup: collapse stale shadow/reconstruction docs and remove obsolete "future" wording after provider integrations land.

## Bottom Line

Sparkle has the right top-level path split now. The next quality jump is editor-visible SIGMA and DLRR, not another broad frame orchestrator refactor. SIGMA needs shadow visibility before lighting, a many-light sampled-light or per-light visibility policy, and a real NRD provider. DLRR needs reconstruction guide buffers and a Streamline DLRR provider, not an upscaler contract stretched sideways. Reference path tracing still needs its own guides, but that comes after the realtime editor results are visible.
