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

1. Hosted CI is intentionally absent. That is cleaner than keeping an inactive GitHub Actions gate; release verification should return only when it is actively owned.
2. Direct lighting is not RTXDI/ReSTIR-shaped yet. It samples one weighted candidate per pixel and has no reservoir history, spatial reuse, light index mapping, or reservoir buffer ownership.
3. Frame graph culling roots only backbuffer writes. When there is no backbuffer root, it keeps every pass alive. Product render outputs should be explicit graph roots.
4. PTLAS support is impressive but research-heavy: there are classic fallback paths, reserved frame-graph buffers, capability/provider selection, future GPU-pack hooks, and a lot of diagnostic state. For release, choose the shipping path and cut or compile-gate the rest.
5. Provider/fallback/diagnostic code is broader than the current product surface. The release path should keep deterministic fallbacks only where they are user-visible behavior, not architecture drivers.
6. The docs are stale in one visible place: `Docs/README.md` still referenced a deleted PBR audit before this document was added.

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
| Providers | `Upscaling`, `RayReconstruction`, `Streamline` | Separate upscaler and ray reconstruction contracts, Streamline-backed NVIDIA providers, fallback providers. |
| Shader ABI | `ShaderRegistrations`, `Tools/Shaders/ShaderCompiler`, `RHI/Public/Shaders` | Typed registrations, package features, reflection, cook artifacts, package inspection. |

## Reference Shape

These are the external shapes Sparkle should be compared against:

| Reference | Recognized shape | Sparkle comparison |
| --- | --- | --- |
| AMD FidelityFX SDK / Cauldron | SDK samples depend on a framework, common render modules, and a custom backend wrapper. Effects remain distinct, sample-driven vertical integrations. | Sparkle has analogous modules, but provider integrations should be treated as product features with one backend bridge, not spread through general renderer state. |
| NVIDIA Donut | `core`, `engine`, `render`, `app` split. Render passes are reusable, app/device management is separate, ray tracing AS ownership is app-specific. | Sparkle has a similar engine/application/render separation. Ray tracing ownership is more ambitious and should be narrowed for release. |
| NVIDIA NVRHI/NRI | Clear RHI tradeoff: either higher-level resource tracking/barriers/lifetime or low-level explicit API with minimal hidden management. Native escape hatches are intentional. | Sparkle sits closer to NVRHI in frame-graph/barrier/resource tracking, with an NRI-like explicit RHI surface underneath. The policy should be documented and tightened. |
| NVIDIA RTXDI | Application owns material model, scene data, ray tracing, GBuffer, graphics API access, light buffers, resource allocation, and shaders. RTXDI supplies sampling/resampling math and bridge callbacks. | Sparkle owns the right surrounding data, but direct lighting has not reached reservoir-backed ReSTIR DI shape. |
| NVIDIA RTXPT / Falcor PathTracer | Path tracing owns path generation, direct-light sampling, path-space decomposition, guide buffers, accumulation/denoising handoff. | Sparkle reference path owns guides now, which is strong. It still lacks reference motion vectors and realtime-grade many-light sampling/cache behavior. |
| NVIDIA Streamline DLSS-RR | Provider requires tagged noisy HDR color and guide buffers, common constants, correct motion/depth conventions, and host command-state restoration. | Sparkle's ray reconstruction contract maps well. It should avoid adding more diagnostic surface and instead finish the missing signal/product cases. |

## Renderer Quality Bar

A great product renderer has these traits:

| Quality | What reviewers look for | Sparkle status |
| --- | --- | --- |
| Feature ownership | Every feature has one owner for settings, resources, passes, shaders, history, and fallback behavior. | Partial. Reference and provider contracts are good; direct lighting and PTLAS are spread across several policy layers. |
| Product graph | Render graph resources are declared, rooted by explicit outputs, culled predictably, and scheduled from data dependencies. | Good foundation, but root selection is too backbuffer-centric. |
| One writer per product | History, guide buffers, GBuffer products, and lighting outputs have one clear writer. | Mostly good. Reference guide ownership is especially good. |
| Explicit RHI policy | The engine clearly states what it tracks automatically and what remains explicit. | Present in code; needs product-level documentation. |
| Shader ABI discipline | Shader source, registration, feature flags, reflection, cook cache, package load, and binding layout all line up. | Strong. This is a review strength. |
| Vendor boundaries | NVIDIA/AMD SDKs are isolated behind provider contracts and capability gates. | Good direction. Cleanup should reduce fallback and diagnostic sprawl. |
| Physical lighting | Direct, indirect, reference, and realtime lighting share BRDF/material/light policy. | Good sharing. Direct many-light strategy is the missing production piece. |
| Temporal ownership | Motion vectors, jitter, history reset, exposure, upscaler/reconstruction state are explicit. | Good resets; reference path lacks motion vectors for reconstruction. |
| Runtime confidence | Build steps, smoke paths, and demo project are present and current. | Local CMake targets and launcher flows exist; hosted CI is intentionally absent. |

## Highest Pain Points

### P0. Release Verification Is Local-Only

Evidence:

- No files are present under `.github/workflows`.
- Existing local verification entry points include:
  - `ShaderCompiler`
  - `ShaderCompilerCliValidation`
  - `architecture_boundary_check`

Why reviewers care:

- Inactive hosted CI is worse than no hosted CI because it creates false release confidence.
- A product release still needs a repeatable verification contract, even if it starts as local CMake and launcher commands.

Cleanup action:

- Keep GitHub Actions absent until the team actively uses it.
- When hosted CI returns, make it target-based and small. Prefer named CMake targets owned near the code they validate.

### P0. Direct Lighting Is Still A One-Candidate Slice

Evidence:

- `DirectLightSampling.hlsli` computes total light weight by looping over all direct lights per pixel.
- `DirectShadowSignal.hlsl` samples one candidate, traces visibility for that candidate, and stores one packed light candidate plus one visibility signal.
- `DirectLighting.hlsl` unpacks that candidate and shades one direct-light sample.

Why reviewers care:

- NVIDIA RTXDI/ReSTIR DI reviewers expect reservoir buffers, temporal/spatial reuse, light-index mapping, selected-sample visibility, and final shading using reservoir weight/PDF.
- Sparkle has the surrounding renderer ownership RTXDI needs, but not the algorithmic product path.

Cleanup action:

- Either finish ReSTIR-style direct lighting as a real vertical slice, or explicitly label the current path as a simple stochastic direct-light sampler and do not present it as RTXDI-grade.
- Avoid adding a "shadow denoiser" as a substitute. The missing feature is many-light sampling/reuse, not more diagnostics.

### P0. Frame Graph Outputs Are Not Explicit Product Roots

Evidence:

- `FrameGraphCompiler::GetRootPassReason()` currently roots passes only if they write the backbuffer.
- If the graph has passes but no roots, `CullDeadPasses()` marks every pass alive.
- Editor/offscreen paths can request viewport products, but the graph root model does not express those as exports.

Why reviewers care:

- A release renderer should know exactly why a pass runs.
- "No root means run everything" is useful during bring-up, but it hides dead work and makes graph correctness harder to review.

Cleanup action:

- Add explicit graph roots for `FrameAssemblyViewportProducts` and provider outputs, or model exported products as roots.
- Delete the fallback that keeps every pass alive once product roots exist.

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

### P1. Provider Surface Is Good, But Fallbacks Shape Too Much Code

Evidence:

- Upscaling and ray reconstruction both maintain active provider plus frame fallback provider.
- Ray reconstruction has a good separate input contract and provider pass, but provider diagnostics are not surfaced through `RendererImageProviderStack` the same way upscaler diagnostics are.
- `RendererImageProviderStack::GetFrameGraphKey()` only keys on ray reconstruction mode; upscaler setting changes are not graph-keyed even though provider execution mode can affect final color production.

Why reviewers care:

- Provider abstraction is a strength when it is small and decisive.
- If fallback code is larger than the feature code, reviewers read it as unfinished behavior.

Cleanup action:

- Keep deterministic fallback behavior only at the product boundary: "provider unavailable means copy unreconstructed/unupscaled color."
- Collapse duplicated fallback-provider machinery where a simple fallback result would do.
- Do not add new diagnostics to paper over provider ambiguity.

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

### P1. Runtime Renderer Confidence Is Not A Visible CI Matrix

Evidence:

- The repo has smoke tooling in `Tools/Launcher` and `Engine/Application/Private/Validation`.
- Hosted CI is intentionally absent; local CMake targets and launcher flows are the current verification surface.

Why reviewers care:

- Product quality is judged through repeatable workflows.
- The request is not to add more validators, so the right fix is to keep the local verification path small and real.

Cleanup action:

- Make one release smoke path executable from a documented local command first; add hosted CI only when it is actively owned.
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
   - Keep hosted CI absent until it is actively owned.
   - Update docs so every linked doc exists.
   - Run the existing architecture boundary check and shader compiler cook path through checked-in CMake targets.

2. Choose the release renderer target.
   - Decide which backend/provider combinations are product-supported.
   - Compile-gate or runtime-hide unfinished paths.
   - Do not let experimental PTLAS, DLRR, or provider fallback paths define the default renderer.

3. Tighten frame graph product ownership.
   - Add explicit exported output roots.
   - Remove "no roots means all passes alive."
   - Keep one writer for every product and history resource.

4. Finish or demote direct lighting.
   - Product path: implement RTXDI/ReSTIR-style reservoirs and reuse.
   - Cleanup path: demote current one-candidate sampler to a simple stochastic mode and stop presenting it as reference-quality direct lighting.

5. Trim provider code.
   - Keep `UpscalerInputContract` and `RayReconstructionInputContract` separate.
   - Collapse fallback-provider duplication where possible.
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
- Do not let fallback providers become the main architecture.
- Do not claim RTXDI/ReSTIR quality until reservoirs, temporal/spatial reuse, and selected-sample visibility exist.
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
