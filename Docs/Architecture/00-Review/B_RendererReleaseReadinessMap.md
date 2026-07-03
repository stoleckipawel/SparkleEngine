# B. Renderer Release Readiness Map

Status: source-backed first pass  
Date: 2026-07-03  
Scope: renderer, RHI, shader pipeline, ray tracing, vendor providers, release review shape

## Intent

This document maps SparkleEngine as it exists now and compares the renderer against the shape reviewers will recognize from AMD and NVIDIA reference repositories. The goal is not to add more validators, panels, or diagnostic scaffolding. The goal is to find the places where product-quality cleanup should delete code, collapse ambiguity, finish vertical slices, and leave less code with stronger ownership.

## Executive Read

Sparkle already has a credible modern-renderer skeleton:

- RHI is split into common contracts plus D3D12 and Vulkan backend-private targets.
- Renderer has a frame graph, pass declarations, transient resource aliasing, shader package contracts, provider contracts, and separate realtime/reference paths.
- Reference path tracing now owns its own guide outputs instead of borrowing realtime GBuffer products.
- Upscaling and ray reconstruction are separate provider categories, which matches NVIDIA/AMD integration expectations better than mixing them into one "upscaler" bucket.

The biggest release-review risks are not TODO comments. The renderer/RHI/shader compiler scan did not find TODO/FIXME/HACK markers. The risks are architectural:

1. Direct lighting now has a ReSTIR DI-shaped vertical slice: reservoir payloads, temporal reuse, spatial reuse, selected-sample visibility, and final reservoir-weighted shading. It still needs image/perf tuning against RTXDI-style reference behavior before it should be marketed as RTXDI-equivalent.
2. Frame graph product ownership now has explicit roots for viewport/provider outputs, and the old no-root culling fallback is removed. Remaining work is to keep product exports request-aware as the editor/offscreen surface grows.
3. PTLAS support is impressive but research-heavy: there are classic fallback paths, reserved frame-graph buffers, capability/provider selection, future GPU-pack hooks, and a lot of diagnostic state. For release, choose the shipping path and cut or compile-gate the rest.
4. Provider fallback ownership is now slimmer: unavailable upscaling or ray reconstruction returns a fallback result, and the frame pass performs the copy. Diagnostic CVars, startup reports, and Streamline log-directory plumbing were removed from this path.
5. The docs are stale in one visible place: `Docs/README.md` still referenced a deleted PBR audit before this document was added.

## Engine Map

Top-level shape:

| Area | Current role | Release review note |
| --- | --- | --- |
| `Engine/Core` | Math, events, logging, diagnostics, utility foundation. | Keep small and dependency-free. |
| `Engine/Platform` | Window/input/platform services. | Good bottom-layer boundary. |
| `Engine/RHI` | Common RHI contracts plus D3D12/Vulkan implementations. | Strong split; public API is broad and should stay explicit. |
| `Engine/Renderer` | Frame graph, passes, scene data, ray tracing, providers, shaders. | Main cleanup target. |
| `Engine/GameFramework` | Runtime scene/assets/components. | Renderer consumes runtime scene data; keep import/cooking out. |
| `Engine/Editor` | Panels and viewport/editor UI. | Should not become renderer policy owner. |
| `Engine/Application` | Runtime/editor hosts plus smoke flows. | Validation/smoke code is large relative to product host. |
| `Tools/Shaders` | Shader compiler, reflection, contracts, cook cache, inspection. | One of the strongest product-looking pieces. |
| `Tools/Cooking`, `Tools/Import` | Asset and source pipeline. | Useful, but release scope needs clear supported path. |
| `Tools/Launcher` | Launch/build/cook/smoke orchestration. | Useful local orchestration; hosted CI should not be reintroduced until it is actively owned. |
| `Projects/Showcase` | Main sample/content project. | Release smoke target and reviewer demo path. |

Renderer flow:

| Stage | Current owner | Notes |
| --- | --- | --- |
| Host frame orchestration | `FramePipeline` | Prepare, record, submit, resize/history/provider resets. Central but readable. |
| Static graph construction | `Frame/Core/Frame.cpp` | Common resources, ray tracing infrastructure, deferred or reference path, post, debug, presentation. |
| Frame data | `BuildFrameContext` in `Frame/Core/FrameContext.cpp` | Builds scene data, mesh data, ray tracing plan, hit data, skinning, lighting, temporal view data. |
| Resource layout | `Frame/Core/FrameAssembly.h` | One broad struct currently carries imported, transient, persistent, history, provider, viewport products. |
| Frame graph | `FrameGraph/*` | Pass declaration, compile, dependency ordering, barriers, transient aliasing, execution. |
| Deferred path | `Frame/Deferred`, `Frame/Lighting`, `Passes/Deferred`, shaders | GBuffer, shadow signal, direct lighting, indirect diffuse/specular, composite, sky. |
| Reference path | `Frame/Reference`, `Passes/Reference`, `Passes/Reference/ReferencePathTracing.hlsl` | Owns color, direct/indirect splits, depth, normal, albedo, material, path guide outputs. |
| Ray tracing scene | `RayTracing/Scene`, `RayTracing/Acceleration`, `Frame/RayTracing` | BLAS cache, classic TLAS, partitioned TLAS, frame-graph build passes. |
| Providers | `Upscaling`, `RayReconstruction`, `Streamline` | Separate upscaler and ray reconstruction contracts, Streamline-backed NVIDIA providers, frame-pass copy fallback. |
| Shader ABI | `ShaderRegistrations`, `Tools/Shaders/ShaderCompiler`, `RHI/Public/Shaders` | Typed registrations, package features, reflection, cook artifacts, package inspection. |

## Reference Shape

These are the external shapes Sparkle should be compared against:

| Reference | Recognized shape | Sparkle comparison |
| --- | --- | --- |
| AMD FidelityFX SDK / Cauldron | SDK samples depend on a framework, common render modules, and a custom backend wrapper. Effects remain distinct, sample-driven vertical integrations. | Sparkle has analogous modules, but provider integrations should be treated as product features with one backend bridge, not spread through general renderer state. |
| NVIDIA Donut | `core`, `engine`, `render`, `app` split. Render passes are reusable, app/device management is separate, ray tracing AS ownership is app-specific. | Sparkle has a similar engine/application/render separation. Ray tracing ownership is more ambitious and should be narrowed for release. |
| NVIDIA NVRHI/NRI | Clear RHI tradeoff: either higher-level resource tracking/barriers/lifetime or low-level explicit API with minimal hidden management. Native escape hatches are intentional. | Sparkle sits closer to NVRHI in frame-graph/barrier/resource tracking, with an NRI-like explicit RHI surface underneath. The policy should be documented and tightened. |
| NVIDIA RTXDI | Application owns material model, scene data, ray tracing, GBuffer, graphics API access, light buffers, resource allocation, and shaders. RTXDI supplies sampling/resampling math and bridge callbacks. | Sparkle owns the surrounding renderer data and now has a native reservoir-backed ReSTIR DI-shaped path. It is not a vendored RTXDI SDK integration and still needs tuning/profiling against RTXDI reference behavior. |
| NVIDIA RTXPT / Falcor PathTracer | Path tracing owns path generation, direct-light sampling, path-space decomposition, guide buffers, accumulation/denoising handoff. | Sparkle reference path owns guides now, which is strong. It still lacks reference motion vectors and realtime-grade many-light sampling/cache behavior. |
| NVIDIA Streamline DLSS-RR | Provider requires tagged noisy HDR color and guide buffers, common constants, correct motion/depth conventions, and host command-state restoration. | Sparkle's ray reconstruction contract maps well. It should avoid adding more diagnostic surface and instead finish the missing signal/product cases. |

## Renderer Quality Bar

A great product renderer has these traits:

| Quality | What reviewers look for | Sparkle status |
| --- | --- | --- |
| Feature ownership | Every feature has one owner for settings, resources, passes, shaders, history, and fallback behavior. | Partial. Reference and provider contracts are good; direct lighting and PTLAS are spread across several policy layers. |
| Product graph | Render graph resources are declared, rooted by explicit outputs, culled predictably, and scheduled from data dependencies. | Good foundation. Viewport/provider outputs are now explicit product roots; continue tightening request-specific product exports. |
| One writer per product | History, guide buffers, GBuffer products, and lighting outputs have one clear writer. | Mostly good. Reference guide ownership is especially good. |
| Explicit RHI policy | The engine clearly states what it tracks automatically and what remains explicit. | Present in code; needs product-level documentation. |
| Shader ABI discipline | Shader source, registration, feature flags, reflection, cook cache, package load, and binding layout all line up. | Strong. This is a review strength. |
| Vendor boundaries | NVIDIA/AMD SDKs are isolated behind provider contracts and capability gates. | Good direction. Provider fallback is now a product-boundary copy instead of a parallel provider object. |
| Physical lighting | Direct, indirect, reference, and realtime lighting share BRDF/material/light policy. | Good sharing. Direct lighting now has many-light reservoir reuse; tune quality, stability, and performance against reference scenes. |
| Temporal ownership | Motion vectors, jitter, history reset, exposure, upscaler/reconstruction state are explicit. | Good resets; reference path lacks motion vectors for reconstruction. |
| Runtime confidence | Build steps, smoke paths, and demo project are present and current. | Local CMake targets and launcher flows exist; hosted CI is intentionally absent. |

## Local Verification Contract

Hosted CI is intentionally absent. The repo should not carry inactive workflow files just to look release-gated.

- No files are present under `.github/workflows`.
- No repo-level helper folder is used for release verification.
- Current local verification entry points are checked-in CMake targets:
  - `ShaderCompilerCliValidation`, which builds `ShaderCompiler`, runs CLI smoke commands, cooks `ComputeClear`, and inspects the cooked package.
  - `architecture_boundary_check`, which checks renderer/RHI ownership boundaries.

Local review command:

```powershell
cmake -S . -B build/local-review -G "Visual Studio 17 2022" -A x64 `
  -DSPARKLE_ENABLE_CONTENT_PIPELINE=OFF `
  -DSPARKLE_ENABLE_SHADER_COMPILER=ON `
  -DSPARKLE_ENABLE_KTX_SUPPORT=OFF `
  -DSPARKLE_ENABLE_NVIDIA_STREAMLINE=OFF `
  -DSPARKLE_RHI_WITH_D3D12=ON `
  -DSPARKLE_RHI_WITH_D3D12_NVAPI=OFF `
  -DSPARKLE_RHI_WITH_VULKAN=OFF

cmake --build build/local-review --config DevelopmentEditor --target ShaderCompilerCliValidation --parallel
cmake --build build/local-review --config DevelopmentEditor --target architecture_boundary_check --parallel
```

Reopen hosted CI only when it is actively owned, target-based, and small. This item is no longer an open P0. The remaining release-confidence work is to choose one documented runtime smoke path.

## Highest Pain Points

### Closed P0. Direct Lighting ReSTIR DI Vertical Slice Is In Place

Evidence:

- `DirectLightReservoir.hlsli` owns the reservoir math: initial candidate streaming, reservoir combine, target PDF evaluation, temporal/spatial M clamping, packed sample/weight payloads, and final shading weight.
- `DirectLightReservoirTemporal.hlsl` builds an initial many-light reservoir and reprojects previous-frame reservoir state through motion vectors when direct-light reservoir history is valid.
- `DirectLightReservoirSpatial.hlsl` combines compatible neighbor reservoirs and writes the current persistent sample, weight, and surface history.
- `DirectShadowSignal.hlsl` traces visibility only for the selected reservoir sample.
- `DirectLighting.hlsl` replays the selected sample and shades with `WeightSum / (M * targetPdf)`.
- `FramePipelineDirectLightReservoirHistory.cpp` owns persistent reservoir sample, weight, and surface textures across frames.

Why reviewers care:

- NVIDIA RTXDI/ReSTIR DI reviewers expect reservoir buffers, temporal/spatial reuse, light-index mapping, selected-sample visibility, and final shading using reservoir weight/PDF.
- Sparkle now has the algorithmic vertical slice and renderer ownership RTXDI-style review expects, without adding a diagnostic or denoiser substitute.

Remaining cleanup:

- Tune candidate counts, neighbor policy, confidence/stability, and quality/performance against representative many-light scenes.
- Keep this documented as a native ReSTIR DI-shaped implementation unless the NVIDIA RTXDI SDK is actually integrated.
- Avoid adding a "shadow denoiser" as a substitute for many-light sampling quality.

### Closed P0. Frame Graph Outputs Are Explicit Product Roots

Evidence:

- `FrameGraphPlan` now carries `productRoots` entries with product names and resource handles.
- `FrameGraphBuilder::ExportTexture()` registers `FrameAssemblyViewportProducts` plus upscaler/ray-reconstruction provider outputs from `FrameGraphFactory::Build()`.
- `FrameGraphCompiler::GetRootPassReason()` roots backbuffer writes and final writers of exported products.
- `CullDeadPasses()` no longer keeps every pass alive when a graph has no roots.
- Transient materialization planning prunes unused transient resources after dead-pass culling.

Why reviewers care:

- A release renderer should know exactly why a pass runs.
- "No root means run everything" is useful during bring-up, but it hides dead work and makes graph correctness harder to review. That behavior has been removed.

Remaining cleanup:

- Keep product export registration aligned with `ViewportRenderRequest::RequestedOutputs` if the graph factory starts receiving per-viewport request policy.
- Add new products only through the export list, not by widening culling fallback behavior.

### P1. PTLAS Is Too Broad For A First Product Release Unless It Is The Product

Evidence:

- `RayTracingPartitionedTlasStrategy` owns partitioned TLAS capability selection, D3D12 NVAPI/Vulkan provider paths, classic fallback, resource allocation, update record upload, native operation packing policy, and diagnostic state.
- Frame graph reserves persistent PTLAS buffers with one-byte placeholder sizes and has passes for future GPU-side logical/native operation work.
- Comments state CPU native packing is still performed in build, while some graph passes maintain GPU-side inputs for future GPU pack paths.

Why reviewers care:

- This is interesting technology, but it reads like active research/integration work unless fully demonstrated.
- Product release wants a clear default path with experimental code behind a deliberate feature gate.

Cleanup action:

- Pick one shipping TLAS path for the release.
- Put PTLAS behind a build/runtime feature gate with an explicit support matrix, or remove the future GPU-pack scaffolding until the GPU path is real.

### Closed P1. Provider Fallbacks No Longer Shape The Provider Surface

Evidence:

- Upscaling and ray reconstruction no longer maintain per-frame fallback provider instances.
- `PassthroughUpscalerProvider` and `NoopRayReconstructionProvider` were removed.
- Provider-unavailable and invalid-input cases now return fallback evaluation results; `AddUpscalerEvaluationPass()` and `AddRayReconstructionProviderPass()` keep the deterministic product-boundary copy.
- `RendererImageProviderStack::GetFrameGraphKey()` now includes effective upscaler provider/quality and ray reconstruction mode/quality.
- Provider diagnostic CVars (`r.Upscaler.Diagnostics`, `r.RayReconstruction.Diagnostics`) and Streamline log path setup were removed.
- DLSS startup/capability logging was removed; provider state stays in the capability/evaluation result path.

Why reviewers care:

- Provider abstraction is a strength when it is small and decisive.
- Fallback code is now smaller and no longer reads as a second feature path.

Remaining cleanup:

- Keep deterministic fallback behavior only at the product boundary: "provider unavailable means copy unreconstructed/unupscaled color."
- Keep provider diagnostics as state needed for feature decisions; do not reintroduce runtime logging as a substitute for product behavior.

### Closed P1. Routine Renderer Logging Was Cut Back

Evidence:

- Frame-pipeline begin/end trace logs, GPU-timing trace dumps, temporal history debug/info logs, texture load debug/info logs, shader-runtime ready/package logs, and first GBuffer draw summary logs were removed.
- `PipelineRuntimeKey` existed only to format pipeline INFO logs and was deleted with its formatter implementation.
- `RayTracingSceneDiagnostics` existed only to emit scene summary INFO logs and was removed; ray tracing performance metrics remain in `RayTracingSceneDiagnosticState`.
- Ray traced shadow diagnostic CVar/uniform plumbing was removed because the shader did not consume it.

Why reviewers care:

- Routine logs make the renderer look louder than its feature code and increase maintenance surface without improving shipped behavior.
- Keeping failure-path warnings/errors while deleting trace/info chatter makes the runtime easier to read and keeps review focus on product paths.

### P1. Reference Path Is Architecturally Better Than It Is Algorithmically Complete

Evidence:

- Reference render targets include direct, indirect diffuse/specular, scene color, primary depth, normal, diffuse/specular albedo, material guide, and path sample guide.
- Reference ray reconstruction intentionally refuses to run because motion vectors are invalid.
- Reference direct lighting loops over every light type and traces one direct sample per light per sample.

Why reviewers care:

- Owning reference guides is the right architecture.
- A reference mode should either be high-quality offline/progressive, or clearly scoped as a debug reference. It should not look like a second realtime path with different compromises.

Cleanup action:

- Keep the current guide ownership.
- Either add reference motion vectors and progressive accumulation, or document that reference ray reconstruction is unavailable by design.
- Consider sharing the future many-light sampling strategy between realtime and reference, with reference mode using higher sample counts.

### P1. Runtime Smoke Contract Needs One Local Command

Evidence:

- The repo has smoke tooling in `Tools/Launcher` and `Engine/Application/Private/Validation`.
- Local CMake targets cover shader package validation and architecture boundaries; runtime smoke tooling still needs one named release path.

Why reviewers care:

- Product quality is judged through repeatable workflows.
- The request is not to add more validators, so the right fix is to keep the local runtime smoke path small and real.

Cleanup action:

- Choose one release smoke path and document the exact local command.
- Remove smoke modes that are not part of the release evidence path.

### P2. Documentation And CMake Hygiene Need A Pass

Evidence:

- `Docs/README.md` referenced a deleted PBR audit.
- `Engine/CMakeLists.txt` contains mojibake box-drawing comments.
- `Engine/RHI/CMakeLists.txt` repeats the Vulkan `source_group` block.
- Renderer/RHI CMake relies heavily on recursive globbing, which is convenient but less reviewable than explicit source ownership for product modules.

Why reviewers care:

- These do not decide renderer quality, but they affect trust and navigation.

Cleanup action:

- Keep docs small and current.
- Remove decorative comments that render incorrectly.
- Deduplicate CMake blocks.
- Keep globbing only where the repo has accepted that policy.

## First Cleanup Sequence

1. Make the repository reviewable.
   - Update docs so every linked doc exists.
   - Run the local release verification contract before review.

2. Choose the release renderer target.
   - Decide which backend/provider combinations are product-supported.
   - Compile-gate or runtime-hide unfinished paths.
   - Do not let experimental PTLAS, DLRR, or provider fallback paths define the default renderer.

3. Tighten frame graph product ownership.
   - Keep explicit exported output roots current as products are added.
   - Keep one writer for every product and history resource.

4. Tune direct lighting.
   - Profile the reservoir path on representative many-light scenes.
   - Compare stability and bias against NVIDIA RTXDI/ReSTIR DI expectations.
   - Keep the implementation native and small unless the product explicitly chooses to vendor RTXDI.

5. Trim provider code.
   - Keep `UpscalerInputContract` and `RayReconstructionInputContract` separate.
   - Keep fallback as pass-level copy behavior rather than provider objects.
   - Keep provider resource contracts decisive and small.

6. Freeze ray tracing scope.
   - Ship classic TLAS first unless PTLAS is a named product feature.
   - If PTLAS ships, remove placeholder/future pass scaffolding and prove the path.

7. Make reference mode honest.
   - Keep its self-owned guide outputs.
   - Add missing motion vectors/progressive accumulation or explicitly say reference reconstruction is unsupported.

## What Not To Do

- Do not add more renderer validators or diagnostic panels as a substitute for finishing features.
- Do not add wrapper layers that only rename handles, settings, or provider results.
- Do not let fallback behavior become the main architecture.
- Do not claim RTXDI SDK equivalence unless the SDK is actually integrated and validated against its reference behavior.
- Do not let reference mode consume realtime GBuffer products unless it writes equivalent products itself.
- Do not keep future GPU/SDK scaffolding in the release path unless the feature is shipping.

## Source References

- AMD FidelityFX SDK structure: https://gpuopen.com/manuals/fidelityfx_sdk/getting-started/sdk-structure/
- AMD FSR SDK structure: https://gpuopen.com/manuals/fsr_sdk/samples/getting-started/sdk-structure/
- AMD FidelityFX SDK v2 overview: https://gpuopen.com/amd-fidelityfx-sdk/
- AMD Cauldron: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron
- NVIDIA Donut: https://github.com/NVIDIA-RTX/Donut
- NVIDIA NVRHI programming guide: https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md
- NVIDIA NRI: https://github.com/NVIDIA-RTX/NRI
- NVIDIA RTXDI integration guide: https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md
- NVIDIA RTXPT: https://github.com/NVIDIA-RTX/RTXPT
- NVIDIA Falcor path tracer guide: https://github.com/NVIDIAGameWorks/Falcor/blob/master/docs/usage/path-tracer.md
- NVIDIA Streamline: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA Streamline DLSS-RR guide: https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS_RR.md
