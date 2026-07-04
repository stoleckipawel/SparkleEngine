# 00. ORDERED - Implementation Roadmap

Status: master implementation sequence
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review` and `Docs/Architecture/01-Implementation`
Use this as: the first document to open before doing engine work

## Purpose

This document orders the KEEP, MODIFY, ADD, and REMOVE documents into a staged implementation plan you can follow over many sessions or weeks.

The four supporting documents answer different questions:

- `01_KEEP_PreservedCapabilities.md`: what must survive every change
- `02_MODIFY_RefactorExistingSystems.md`: valuable systems to refactor and harden
- `03_ADD_MinimalMissingCapabilities.md`: narrowly allowed additions
- `04_REMOVE_DeletionsAndCleanup.md`: deletion queue and cleanup prompts

This roadmap tells you when to use each one.

The outcome should be a smaller, sharper renderer-first engine that demonstrates:

- explicit D3D12/Vulkan ownership and parity
- strong RHI and frame-graph architecture
- proper Core, RHI, Renderer, GameFramework, and Tools separation
- thin high-level concepts that keep users away from low-level implementation details without hiding rendering behavior behind excessive abstraction
- classic TLAS and PTLAS as product RT features
- shader compiler/cook/runtime ABI strength
- screenshot/BMP capture as a hardened editor/tool capability
- multi-level project support without depot pollution
- renderer feature depth before profiling work
- professional debugging support
- fewer public observation APIs
- fewer diagnostics, reports, logs, wrappers, and future scaffolding

## How To Use These Documents

For every implementation batch:

1. Read `01_KEEP_PreservedCapabilities.md`.
2. Pick exactly one roadmap stage from this document.
3. Open the referenced section in `02_MODIFY_RefactorExistingSystems.md`, `03_ADD_MinimalMissingCapabilities.md`, or `04_REMOVE_DeletionsAndCleanup.md`.
4. Fill the batch prompt before editing.
5. Make the smallest meaningful code/content change.
6. Verify the acceptance criteria from the stage with targeted checks; defer full build/cook/run stabilization to Stage 42 unless you intentionally choose a checkpoint.
7. Update only existing docs if the implementation result makes current instructions stale.

Do not start by creating more plans. This is the plan.

## Batch Prompt Template

Copy this into your working notes before each batch:

```text
Batch:
Roadmap stage:
Persona pillar:
Capability preserved:
Capability improved:
Primary source docs (KEEP/MODIFY/ADD/REMOVE):
Files/paths to inspect:
Search patterns:
Expected removals:
Allowed additions:
D3D12 impact:
Vulkan impact:
Shader/cook impact:
Runtime impact:
Public/private API impact:
Content impact:
Tooling/capture impact:
No-pollution check:
Verification plan:
Full build/cook/run deferred to:
Done criteria:
```

No-pollution check means:

- no new documentation files unless the user explicitly asks
- no new logs
- no new validation systems
- no new diagnostic panels
- no new report formats
- no new wrapper layers
- no thick abstraction layers
- no future-feature scaffolding
- no uncataloged heavy content

## Renderer-First Module Contract

Sparkle is a compact renderer-first engine. Runtime, editor, content, and tools exist to prove explicit graphics API ownership, shader pipeline quality, cooked content workflows, and advanced rendering features. They should not grow into separate products unless they remove more code or make a preserved capability clearer.

Thin high-level concepts are allowed: project, level, scene, render path, backend, capture, cook, package. These concepts should let users operate the engine without touching low-level implementation details. They must stay thin enough that rendering behavior remains traceable from scene data to renderer pass to frame graph to RHI backend.

| Area | Owns | Does Not Own |
| --- | --- | --- |
| Core | foundation utilities, math, files, config, fatal checks, small platform-neutral helpers | renderer policy, backend resources, workflow UI, content schemas beyond common utilities |
| RHI | explicit D3D12/Vulkan resources, descriptors, pipelines, queues, command lists, barriers, uploads/readbacks, ray tracing, native interop, capture, presentation | scene extraction, material policy, frame scheduling, editor panels, generic wrapper layers |
| Renderer | frame graph, passes, render paths, scene extraction, shader registrations, provider boundaries, TLAS/PTLAS policy, render products | backend-private API details, level ownership, launcher/cooker workflows |
| GameFramework | project runtime concepts, levels, scenes, components, cameras, materials, meshes, lights, cooked scene loading | renderer-private implementation details, RHI resources, shader package compilation |
| Tools | shader compile, import, cook, launch, clean, package-if-owned, asset workflow entrypoints | runtime render ownership, diagnostic cockpit behavior, unowned package products |
| Projects | sample projects, selectable levels, scene/content data, optional heavy content packs | engine contracts, backend-specific behavior, required heavy content in the default repo footprint |

## Phase Overview

| Phase | Theme | Primary doc | Result |
| ---: | --- | --- | --- |
| 0 | Baseline and navigation | This doc + KEEP | You know what must not break. |
| 1 | Repository/doc hygiene | REMOVE category 10 | Review set is navigable and neutral. |
| 2 | Content catalog and optional heavy packs | MODIFY 1 + ADD 1 + REMOVE 1 | Multi-level support remains; depot weight can drop. |
| 3 | Screenshot capture hardening | MODIFY 2 + ADD 3 + REMOVE 2 | Capture preserved; smoke/ad hoc capture removed. |
| 4 | Smoke/report/cook artifact cleanup | REMOVE 2 + REMOVE 3 | Default workflows produce assets, not reports. |
| 5 | Launcher slimming | MODIFY 3 + REMOVE 7 | Launcher becomes workflow shell. |
| 6 | Public observation API narrowing | MODIFY 4 + REMOVE 4 | Public API shrinks around behavior. |
| 7 | TLAS/PTLAS refactor | MODIFY 5 + ADD 2 + REMOVE 5 | Classic TLAS and PTLAS both work; PTLAS is smaller. |
| 8 | Reference path tracing cleanup | MODIFY 6 + REMOVE 8 | Reference mode has one clear role. |
| 9 | Shader debug and duplication cleanup | MODIFY 7 + REMOVE 6 | Shader ABI stays strong; default debug artifacts leave. |
| 10 | Core/GameFramework surface cleanup | MODIFY 8 | Public engine boundaries shrink. |
| 11 | Renderer feature hardening | MODIFY 9 + ADD 5 | RT/GI/post/denoise/upscale/framegraph/shaders/passes become cleaner. |
| 12 | Package contract cleanup | MODIFY 10 + ADD 6 + REMOVE 9 | Package outputs are real and intentional. |
| 13 | Late profiling and measurement | ADD late only + review docs | Measurement happens after feature cleanup. |
| 14 | Final readiness pass | All four docs | Persona and engine goals are demonstrably covered. |

## Reference Repository Map

Use these repositories as implementation references. Do not copy architecture blindly. Use them to compare ownership, directory shape, feature boundaries, and how much code is needed for a mature implementation.

| Ref | Repository | Use For |
| --- | --- | --- |
| NV-DONUT | https://github.com/NVIDIA-RTX/Donut | renderer/app/core split, sample framework boundaries |
| NV-DONUT-SAMPLES | https://github.com/NVIDIA-RTX/Donut-Samples | keeping samples separate from framework code |
| NV-NVRHI | https://github.com/NVIDIA-RTX/NVRHI | higher-level RHI conveniences, binding/resource lifetime style |
| NV-NRI | https://github.com/NVIDIA-RTX/NRI | low-level explicit RHI shape and low-overhead philosophy |
| NV-NRI-SAMPLES | https://github.com/NVIDIA-RTX/NRISamples | small NRI usage/test-bench examples |
| NV-SHADERMAKE | https://github.com/NVIDIA-RTX/ShaderMake | shader compilation tool shape |
| NV-STREAMLINE | https://github.com/NVIDIA-RTX/Streamline | provider/resource tagging and integration boundary |
| NV-RTXDI | https://github.com/NVIDIA-RTX/RTXDI | reservoir direct lighting library/application ownership split |
| NV-RTXDI-LIBRARY | https://github.com/NVIDIA-RTX/RTXDI-Library | library-only reservoir direct lighting split |
| NV-RTXDI-ASSETS | https://github.com/NVIDIA-RTX/RTXDI-Assets | optional sample asset split |
| NV-SHARC | https://github.com/NVIDIA-RTX/SHARC | shader-focused GI feature integration |
| NV-NRD | https://github.com/NVIDIA-RTX/NRD | denoising feature/resource boundary |
| NV-NRD-SAMPLE | https://github.com/NVIDIA-RTX/NRD-Sample | denoising sample integration boundary |
| NV-RTXPT | https://github.com/NVIDIA-RTX/RTXPT | path tracing product/sample split |
| NV-RTXNS | https://github.com/NVIDIA-RTX/RTXNS | neural shading/readiness without broad engine ML ownership |
| NV-RTX-KIT | https://github.com/NVIDIA-RTX/RTX-Kit | neural rendering SDK landing and integration grouping |
| AMD-CAULDRON | https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron | D3D12/Vulkan framework split, sample resource management, shader/pipeline cache |
| AMD-FIDELITYFX | https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK | SDK/sample/tool/content separation |
| AMD-D3D12MA | https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator | D3D12 memory allocator integration |
| AMD-VMA | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | Vulkan memory allocator integration |
| UE-SOURCE | https://github.com/EpicGames/UnrealEngine | RHI/RenderCore/Renderer/GameFramework module separation; access requires linked Epic/GitHub account |

## Source Coverage Matrix

Use this matrix to confirm the implementation sequence covers the `00-Review` source set. If a later edit adds a requirement to a review document, add it to this matrix or to the relevant stage acceptance criteria instead of creating another plan.

| Source Document | Main Requirements Pulled Forward | Roadmap Stages That Close It | Concrete Result Expected |
| --- | --- | --- | --- |
| `A_PrincipalRenderingRequirements.md` | explicit graphics API ownership, shader quality, debugging fluency, real-time rendering depth, advanced feature readiness | 01, 20-35, 39, 41, 42 | Sparkle shows D3D12/Vulkan competence, shader/cook ABI discipline, ray tracing/GI/path tracing feature ownership, and late measured evidence. |
| `D_WholeRepositoryArchitectureMap.md` | current module map, public/private boundaries, RHI/Renderer flow, memory/GPU/CPU maps, system links | 02, 16-24, 35-38, 41, 42 | Public surface shrinks, RHI/Renderer/GameFramework/Core/Tools boundaries become easier to trace, and final checks prove the map improved. |
| `E_ExternalRendererRepositoryComparison.md` | scope clarity, RHI tradeoff honesty, sample/content separation, SDK boundary discipline, product packaging policy | Reference Repository Map, 01, 04-07, 20-23, 24-35, 40 | Sparkle follows mature repository patterns without copying their bulk: explicit scope, optional content, narrow provider bridges, real package outputs. |
| `F_StagedDeletionFirstImprovementPlan.md` | deletion-first work order, no-new-pollution rule, preserve capabilities while removing scaffolding | 03, 07, 10-19, 22, 27, 32-38, 40 | Every batch starts from preserved capability checks and ends with net simplification or a recorded reason why not. |
| `G_AdvancedGraphicsEngineExecutiveSummary.md` | product identity, shader ABI, capture, ray tracing/GI/path tracing, neural readiness, late performance evidence, wording safety | 01, 03, 08-12, 20-35, 39-42 | The engine direction is advanced graphics first: capture and shader packages remain, RT features deepen, ML-adjacent readiness stays practical, profiling waits until features exist. |
| `H_AdvancedGraphicsEngineerPersona.md` | explicit API ownership, renderer feature depth, shader/kernel craft, GPU architecture thinking, debugging/tool fluency, communication through clean code | 01, 20-35, 39, 41, 42 | A reviewer can infer the owner from the repo: thin public concepts, explicit backend work, preserved advanced RT/capture/shader capabilities, and less code around each feature. |

## Granular Implementation Sequence

Use this sequence as the real execution order. The broad phase notes below remain supporting context only; do not treat them as a second roadmap.

### Stage 00: Navigation And Batch Discipline

References: NV-DONUT, AMD-FIDELITYFX, UE-SOURCE.

Prompt:

- Make `Docs/README.md` point to this roadmap first.
- Keep `01-Implementation` as the active prompt set and `00-Review` as source material.
- Use the batch prompt for every change.

Acceptance:

- [x] README opens with `00_ORDERED_ImplementationRoadmap.md`.
- [x] No new planning document is created.
- [x] Every future batch names KEEP/MODIFY/ADD/REMOVE source docs.
- [x] Any final build/cook/run risk is recorded instead of forcing stabilization after each small stage.

### Stage 01: Product Identity And Module Contract

References: NV-DONUT, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Confirm Sparkle is a renderer-first engine.
- Define in existing docs only: Core, RHI, Renderer, GameFramework, Tools, Projects responsibilities.
- Keep high-level concepts thin: project, level, scene, render path, backend, capture, cook.

Acceptance:

- [x] Core owns foundation utilities only.
- [x] RHI owns explicit D3D12/Vulkan resources, descriptors, pipelines, queues, barriers, ray tracing, capture, presentation.
- [x] Renderer owns frame graph, passes, scene extraction, provider boundaries, TLAS/PTLAS policy, shader registrations.
- [x] GameFramework owns level/scene/assets/components without renderer-private details.
- [x] Tools own shader compile, import, cook, launch, package workflows.
- [x] Projects own sample projects, selectable levels, scene/content data, and optional heavy content packs.
- [x] No new module abstraction is introduced.

### Stage 02: Baseline Repository Shape Snapshot

References: NV-DONUT, AMD-FIDELITYFX, UE-SOURCE.

Prompt:

- Capture current source line/file/depot weight by area.
- Record public/private line counts for Core/RHI/Renderer/GameFramework.
- Save this as working notes or update existing review docs only if stale.

Acceptance:

- [x] Current largest areas are known.
- [x] Current public API hotspots are known.
- [x] Current Projects depot weight is known.
- [x] No source code changes are made in this stage.

### Stage 03: Preserve Capability Checklist

References: NV-NRI, NV-NVRHI, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Audit every planned deletion against `01_KEEP_PreservedCapabilities.md`.
- Mark preserved capability owners.

Acceptance:

- [ ] D3D12 and Vulkan parity is listed as a preserved capability.
- [ ] Offline cooked shader packages with reflection data are listed as preserved.
- [ ] Screenshot/BMP capture is listed as preserved.
- [ ] Classic TLAS and PTLAS are listed as preserved.
- [ ] Multi-level support is listed as preserved.
- [ ] Core/RHI/Renderer/GameFramework/Tools/Projects separation is listed as preserved.

### Stage 04: Content Discovery And Default Level Set

References: NV-DONUT-SAMPLES, AMD-FIDELITYFX, AMD-CAULDRON.

Prompt:

- Inventory `Projects/Showcase` levels, scenes, assets, and heavyweight media.
- Decide the curated default level set.
- Do not remove content yet.

Acceptance:

- [ ] Every in-repo level is listed.
- [ ] Default level set is named.
- [ ] Heavy optional content candidates are listed by path and approximate size.
- [ ] Multi-level support remains explicitly preserved.

### Stage 05: Minimal Level/Content Catalog

References: NV-DONUT-SAMPLES, AMD-FIDELITYFX.

Prompt:

- Add only the metadata needed to select levels and identify optional packs.
- Keep the schema tiny.

Acceptance:

- [ ] Catalog contains level id, display name, source path, default inclusion, optional pack id if needed.
- [ ] Launcher/cooker/runtime can resolve the default level set through the catalog.
- [ ] Missing optional pack state can be represented.
- [ ] No asset database or content browser rewrite is added.

### Stage 06: Optional Heavy Content Pack Boundary

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES, NV-RTXDI-ASSETS if used through RTXDI asset split.

Prompt:

- Define optional content pack ownership.
- Make heavy content optional without reducing level capability.

Acceptance:

- [ ] Optional pack root is defined.
- [ ] Default build/cook/run path does not require optional heavy content.
- [ ] Missing optional content produces a clear non-fatal state.
- [ ] Core repo byte reduction target is recorded.

### Stage 07: Externalize Or Remove Heavy Content

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- Move/delete heavy content after catalog support exists.
- Preserve curated levels.

Acceptance:

- [ ] Heavy Bistro/media content is removed from default repo footprint or marked external.
- [ ] Default level set remains selectable.
- [ ] Optional heavy levels remain discoverable.
- [ ] Depot byte count is materially lower.

### Stage 08: Capture Path Inventory

References: AMD-CAULDRON, UE-SOURCE, NV-DONUT.

Prompt:

- Find every screenshot/BMP/readback path.
- Separate product capture from smoke/ad hoc capture.

Acceptance:

- [ ] Every `CaptureTextureToBmp`, `RhiCaptureService`, and BMP writer call site is listed.
- [ ] One intended product capture owner is selected.
- [ ] Smoke/ad hoc capture call sites are named for removal.
- [ ] D3D12/Vulkan readback impact is recorded.

### Stage 09: Harden Screenshot/BMP Capture

References: AMD-CAULDRON, UE-SOURCE.

Prompt:

- Preserve capture as an editor/tool capability.
- Narrow ownership and remove smoke coupling.

Acceptance:

- [ ] Capture entrypoint is product-owned.
- [ ] BMP writer remains only behind the intended path.
- [ ] Smoke/ad hoc capture ownership is gone.
- [ ] Public API is smaller or explicitly justified.

### Stage 10: Smoke Harness Removal

References: NV-NRI-SAMPLES for test-bench separation, AMD-CAULDRON for framework simplicity.

Prompt:

- Remove smoke harnesses that preserve validation/report scaffolding.
- Preserve fatal checks and product launch paths.

Acceptance:

- [ ] `RhiSmoke`, `SmokeDiagnostics`, and `SPARKLE_SMOKE` references are gone or intentionally product-owned.
- [ ] No screenshot capability is removed.
- [ ] No new validation system replaces smoke.
- [ ] Final stabilization risk is recorded if launch behavior is touched.

### Stage 11: AssetCooker Default Report Cleanup

References: AMD-FIDELITYFX, NV-SHADERMAKE.

Prompt:

- Remove default cook reports and timing summaries.
- Keep cooked asset outputs and fatal errors.

Acceptance:

- [ ] Default AssetCooker path writes cooked assets, not plan/timing report artifacts.
- [ ] `asset-cooker-plan-v1` and `asset-cooker-summary-v1` are gone from default path.
- [ ] Fatal errors remain clear.
- [ ] No report replacement is added.

### Stage 12: Shader Debug Artifact Cleanup

References: NV-SHADERMAKE, AMD-CAULDRON.

Prompt:

- Remove default shader debug bundles/stats.
- Preserve offline cooked packages and reflection data.

Acceptance:

- [ ] Default shader cook writes runtime packages.
- [ ] Reflection data remains available to runtime.
- [ ] Debug artifacts are opt-in or removed.
- [ ] Validation-only shader registrations are removed from product runtime.

### Stage 13: Launcher Workflow Inventory

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- List launcher workflows that are product-owned.
- Mark diagnostic cockpit features for removal.

Acceptance:

- [ ] Build, cook, run, clean, package-if-owned are the only first-class launcher workflows.
- [ ] Diagnostic/status/quality/debug-only launcher pages are listed.
- [ ] Package UI ownership is decided.
- [ ] No launcher feature is added.

### Stage 14: Launcher Workflow Slimming

References: AMD-FIDELITYFX, NV-DONUT-SAMPLES.

Prompt:

- Remove launcher UI/actions that do not support current workflows.

Acceptance:

- [ ] Launcher source line count decreases.
- [ ] Shader debug/stat toggles leave default GUI.
- [ ] Diagnostic-only pages/actions are removed.
- [ ] No new panel replaces deleted panels.

### Stage 15: Package Ownership Decision

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Decide which packages are real: runtime, editor, launcher, dev tools, symbols, optional content.

Acceptance:

- [ ] Owned package list is written in existing docs or package config.
- [ ] Unowned package outputs are marked for removal.
- [ ] Optional content is not part of runtime package by default.
- [ ] Checksums/manifests are kept only if consumed.

### Stage 16: Public Renderer Observation Inventory

References: UE-SOURCE, NV-DONUT, AMD-CAULDRON.

Prompt:

- Inventory public Renderer diagnostics and observation APIs.

Acceptance:

- [ ] Renderer public diagnostic headers are listed.
- [ ] Consumers are listed.
- [ ] Editor-only consumers are separated from runtime behavior.
- [ ] Product-owned observation surfaces are explicitly justified.

### Stage 17: Public RHI Observation Inventory

References: NV-NRI, NV-NVRHI, UE-SOURCE.

Prompt:

- Inventory RHI public diagnostics, descriptor snapshots, memory snapshots, capture surfaces.

Acceptance:

- [ ] RHI public observation APIs are listed.
- [ ] Runtime-critical facts are separated from report/detail dumps.
- [ ] Capture is preserved and separated from broad diagnostics.
- [ ] D3D12/Vulkan parity risk is recorded.

### Stage 18: Move Or Remove Editor-Only Diagnostics

References: UE-SOURCE, NV-DONUT.

Prompt:

- Move editor-only diagnostics behind editor-private ownership or remove unowned panels.

Acceptance:

- [ ] Public Renderer/RHI diagnostics line count decreases.
- [ ] Editor panels are either product-owned or removed.
- [ ] No diagnostics facade is added.
- [ ] Runtime pressure facts remain only if consumed.

### Stage 19: Compact Runtime Pressure Facts

References: NV-NVRHI, AMD-D3D12MA, AMD-VMA.

Prompt:

- Keep compact memory/descriptor pressure facts only when they drive runtime policy.

Acceptance:

- [ ] Memory budget facts are backed by D3D12MA/VMA paths.
- [ ] Descriptor pressure facts are consumed or removed.
- [ ] Public detail dumps are removed from default API.
- [ ] No report format is added.

### Stage 20: RHI Service Boundary Audit

References: NV-NRI, NV-NVRHI, UE-SOURCE.

Prompt:

- Audit `RenderHardwareInterface` services and ownership.
- Keep RHI explicit and thin.

Acceptance:

- [ ] Resource, descriptor, pipeline, upload, ray tracing, interop, capture, diagnostics, presentation services are still owned by RHI.
- [ ] Backend-native details remain private or provider-bridged.
- [ ] No new RHI wrapper layer is added.
- [ ] Public service surface is smaller or justified.

### Stage 21: D3D12/Vulkan Parity Matrix

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Create a working parity matrix in existing docs or issue notes for key RHI/Renderer features.

Acceptance:

- [ ] D3D12/Vulkan parity is named for resources, descriptors, pipelines, uploads, presentation, ray tracing, PTLAS, capture.
- [ ] Backend-specific gaps are listed.
- [ ] No parity gap is hidden behind generic abstraction.
- [ ] Any extension opportunity is recorded.

### Stage 22: RHI Resource/Descriptor/Pipeline Cleanup

References: NV-NRI, NV-NVRHI, AMD-CAULDRON.

Prompt:

- Remove duplicate helper paths while preserving explicit API behavior.

Acceptance:

- [ ] Resource creation path remains backend-owned.
- [ ] Descriptor binding model remains understandable.
- [ ] Pipeline/layout creation remains explicit.
- [ ] Any removed helper had no unique product behavior.

### Stage 23: Native Interop Boundary

References: NV-NVRHI, NV-STREAMLINE, UE-SOURCE.

Prompt:

- Keep native interop only for explicit provider bridges.

Acceptance:

- [ ] Native handle access has explicit consumer ownership.
- [ ] Provider resource contracts remain narrow.
- [ ] No broad native escape hatch is exposed casually.
- [ ] Streamline/upscaling/ray reconstruction paths remain functional where supported.

### Stage 24: Shader Compiler/Cook ABI Audit

References: NV-SHADERMAKE, AMD-CAULDRON, UE-SOURCE.

Prompt:

- Audit shader source-to-cooked-package-to-runtime flow.

Acceptance:

- [ ] Shader source includes are tracked.
- [ ] DXC/Slang target outputs are known.
- [ ] Reflection data survives cook.
- [ ] Runtime package cache loads cooked packages.
- [ ] Debug artifact behavior is opt-in or removed.

### Stage 25: Shader Registration And Binding Duplication Cleanup

References: NV-SHADERMAKE, NV-NVRHI, UE-SOURCE.

Prompt:

- Reduce duplicated C++ registration/HLSL binding declarations only when net code decreases.

Acceptance:

- [ ] Duplicate declarations are listed.
- [ ] Any simplification deletes more code than it adds.
- [ ] No code generator is added unless net code decreases materially.
- [ ] Layout verification remains intact.

### Stage 26: Classic TLAS Flow Audit

References: NV-NRI, UE-SOURCE, NV-RTXPT.

Prompt:

- Trace classic BLAS/TLAS build/update/trace ownership.

Acceptance:

- [ ] BLAS lifecycle owner is clear.
- [ ] Classic TLAS lifecycle owner is clear.
- [ ] Trace pipeline usage is clear.
- [ ] D3D12/Vulkan paths are both represented where supported.

### Stage 27: PTLAS Minimal Reference Flow

References: NV-NRI, UE-SOURCE, NV-RTXPT.

Prompt:

- Reduce PTLAS to capability check, compact descriptor input, backend build/update, resource lifetime, trace use.

Acceptance:

- [ ] PTLAS planner metrics not required for product behavior are removed.
- [ ] CPU validation readbacks not required for product behavior are removed.
- [ ] Future GPU-pack placeholders are removed.
- [ ] PTLAS still renders where supported.

### Stage 28: TLAS/PTLAS User Selection

References: NV-NRI, UE-SOURCE.

Prompt:

- Preserve user-facing selection between classic TLAS and PTLAS.

Acceptance:

- [ ] Selection is available through one clear policy point.
- [ ] Unsupported backend/feature state fails gracefully or falls back explicitly.
- [ ] No debug panel is added for selection.
- [ ] Selection code is smaller or clearer than before.

### Stage 29: PTLAS D3D12/Vulkan Backend Parity

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Preserve or extend PTLAS capability across D3D12 and Vulkan.

Acceptance:

- [ ] D3D12 PTLAS support path is listed and functional where supported.
- [ ] Vulkan PTLAS support path is listed and functional where supported.
- [ ] Capability checks are explicit per backend.
- [ ] PTLAS is not made D3D12-only if Vulkan support exists.

### Stage 30: Reference Path Tracing Role

References: NV-RTXPT, AMD-CAULDRON.

Prompt:

- Choose debug reference as the first clear role.

Acceptance:

- [ ] Reference path has one sentence role.
- [ ] Provider handoff hooks unsupported by data are removed.
- [ ] Guide outputs without consumers are removed.
- [ ] Material/light policy aligns with realtime path.

### Stage 31: Reservoir Direct Lighting Cleanup

References: NV-RTXDI, NV-RTXDI-LIBRARY if used, NV-SHARC.

Prompt:

- Keep native reservoir-based direct lighting honest and owned by Sparkle.

Acceptance:

- [ ] Direct lighting is not described as SDK-equivalent unless SDK is integrated.
- [ ] Light buffers, GBuffer addressing, TLAS, material model, and shader scheduling remain Sparkle-owned.
- [ ] Unused debug views or report outputs are removed.
- [ ] Shader/resource ownership is obvious.

### Stage 32: Post-Processing Pass Ownership

References: AMD-CAULDRON, NV-DONUT.

Prompt:

- Make post-processing pass inputs/outputs explicit and reduce duplicate pass plumbing.

Acceptance:

- [ ] Each post-process pass declares inputs/outputs clearly.
- [ ] Unused settings/CVars are removed.
- [ ] No new post-processing framework is added.
- [ ] Existing output quality path remains buildable.

### Stage 33: Denoising Feature Boundary

References: NV-NRD, NV-NRD-SAMPLE, AMD-FIDELITYFX.

Prompt:

- Treat denoising as a renderer feature slice with explicit resources.

Acceptance:

- [ ] Denoiser inputs/outputs are named.
- [ ] History/resource ownership is explicit.
- [ ] Provider boundary is narrow if external denoiser integration exists.
- [ ] No diagnostic panel is added.

### Stage 34: Upscaling And Ray Reconstruction Boundary

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Preserve upscaling/ray reconstruction provider capability while trimming diagnostics and fallback scaffolding.

Acceptance:

- [ ] Provider required resources are narrow and named.
- [ ] Depth, motion vectors, exposure, history, jitter, frame index, and camera state ownership is clear where used.
- [ ] Unused fallback/provider objects are removed.
- [ ] Streamline bridge remains narrow.

### Stage 35: Frame Graph Pass/Resource Ownership

References: UE-SOURCE, NV-DONUT, NV-NVRHI.

Prompt:

- Reduce frame graph/pass duplication without replacing the graph.

Acceptance:

- [ ] Frame graph remains the only render scheduling abstraction.
- [ ] Pass inputs/outputs/history dependencies are clearer.
- [ ] Transient/persistent resource ownership is easier to trace.
- [ ] No replacement render graph is added.

### Stage 36: Core Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Reduce public Core convenience APIs that are not stable contracts.

Acceptance:

- [ ] Public Core line count decreases.
- [ ] Private helpers move private.
- [ ] No replacement aggregate header appears.
- [ ] Platform/Renderer/GameFramework includes remain clean.

### Stage 37: GameFramework Public API Cleanup

References: UE-SOURCE, NV-DONUT.

Prompt:

- Keep GameFramework high-level user concepts while hiding implementation details.

Acceptance:

- [ ] Level, scene, component, asset concepts remain.
- [ ] Asset loader/manifest implementation details are private where possible.
- [ ] Multi-level support remains intact.
- [ ] Renderer still consumes GameFramework privately.

### Stage 38: Import/Cooker Public Surface Cleanup

References: AMD-CAULDRON, AMD-FIDELITYFX.

Prompt:

- Keep tool public headers only when another executable consumes them as stable API.

Acceptance:

- [ ] Import/cooker public headers are audited.
- [ ] Unused public bridge headers are removed.
- [ ] Cooked asset outputs remain.
- [ ] Default reports do not return.

### Stage 39: Neural Rendering Readiness Without ML Bloat

References: NV-RTXNS, NV-RTX-KIT, NV-NRD, NV-STREAMLINE.

Prompt:

- Prepare for inference-like shader features without adding runtime ML frameworks.

Acceptance:

- [ ] Slang/HLSL flexibility is preserved.
- [ ] Tensor/operator concepts remain design-level unless a renderer feature needs them.
- [ ] No PyTorch/TensorFlow/ONNX Runtime dependency is added.
- [ ] Denoising/upscaling/ray reconstruction paths remain the practical readiness surface.

### Stage 40: Package Contract Implementation

References: NV-STREAMLINE, AMD-FIDELITYFX.

Prompt:

- Make package outputs real and intentional.

Acceptance:

- [ ] Runtime/editor/launcher/dev tools/symbols/optional content package ownership is decided.
- [ ] Unowned package assembly code is removed.
- [ ] Optional content is separate from runtime package.
- [ ] Manifest/checksum fields are consumed or removed.

### Stage 41: Late Measurement Setup

References: AMD-CAULDRON, NV-NRI, UE-SOURCE.

Prompt:

- Use existing markers/timestamps/debuggers before adding any measurement code.

Acceptance:

- [ ] Feature cleanup is complete for the measured path.
- [ ] Existing PIX/RenderDoc/Nsight hooks are used first.
- [ ] Any new measurement code replaces old diagnostics.
- [ ] No new report panel or benchmark format appears by default.

### Stage 42: Final Stabilization And Persona Evidence

References: all reference repos above.

Prompt:

- Run final stabilization after the staged cleanup sequence.
- Confirm the repo demonstrates the persona.

Acceptance:

- [ ] Build relevant editor/runtime targets.
- [ ] Cook curated default level set.
- [ ] Run default level set.
- [ ] Run D3D12 path where supported.
- [ ] Run Vulkan path where supported.
- [ ] Verify shader packages cook and load with reflection data.
- [ ] Verify screenshot/BMP capture.
- [ ] Verify classic TLAS selection.
- [ ] Verify PTLAS selection where supported.
- [ ] Verify multiple levels remain selectable.
- [ ] Confirm public APIs are smaller.
- [ ] Confirm repo/depot weight is smaller.
- [ ] Confirm no new docs/logs/validation/report systems/wrappers/thick abstractions replaced old ones.

## Phase Note 0: Baseline And Guardrails

Goal:

- establish what must be preserved before changing code
- stop accidental deletion of important capabilities

Use:

- `01_KEEP_PreservedCapabilities.md`
- `H_AdvancedGraphicsEngineerPersona.md`
- `G_AdvancedGraphicsEngineExecutiveSummary.md`

Implementation prompt:

1. Read `01_KEEP_PreservedCapabilities.md`.
2. For the subsystem you want to touch, list preserved capabilities from the KEEP doc.
3. Run discovery searches before editing.
4. Save the final build/cook/run commands you will use in Stage 42.

Baseline searches:

```powershell
rg -n "D3D12|Vulkan|PTLAS|PartitionedTlas|CaptureTextureToBmp|RhiCaptureService|debug-artifacts|asset-cooker-summary|asset-cooker-plan|SmokeDiagnostics|SPARKLE_SMOKE" Engine Tools Projects
rg -n "RendererMemoryDiagnostics|MeshDiagnostics|TextureDiagnostics|RhiDiagnostics|DescriptorUsage|ShaderStats|ComputeClear|HelloWorld" Engine Tools
```

Must preserve:

- D3D12 backend
- Vulkan backend
- D3D12/Vulkan parity preserved or extended
- frame graph
- offline cooked shader packages with reflection data
- classic TLAS
- PTLAS
- screenshot/BMP capture
- multi-level support
- PIX/RenderDoc/Nsight markers
- final build/cook/run workflows

Done criteria:

- [ ] You know the verification command for the target subsystem.
- [ ] You know the final build/cook/run commands for Stage 42.
- [ ] You know which KEEP items the batch must preserve.
- [ ] You have identified whether the batch is MODIFY, ADD, REMOVE, or mixed.

## Phase Note 1: Repository And Documentation Hygiene

Goal:

- make the docs navigable
- keep `01-Implementation` as the active execution spine
- keep `00-Review` as source material
- remove stale review references

Use:

- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 10
- `Docs/README.md`

Implementation prompt:

1. Keep `Docs/README.md` pointing to this roadmap first.
2. Keep the four implementation docs as the active prompt set.
3. Keep review docs as source material only.
4. Remove stale references to deleted docs.
5. Do not create another planning layer.

Search patterns:

```powershell
rg -n "B_RendererReleaseReadinessMap|C_ValidationDiagnosticsCleanupMap|stale review doc|deleted review doc|source trail" Docs
```

Done criteria:

- [ ] README points to `00_ORDERED_ImplementationRoadmap.md` first.
- [ ] No stale links to deleted review docs remain.
- [ ] No sensitive source trail remains.
- [ ] No new planning document is created after this roadmap.

## Phase Note 2: Content Catalog And Optional Heavy Packs

Goal:

- preserve multiple levels
- reduce depot weight
- stop heavy content from polluting default build/cook/run

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 1
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 1
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 1
- `01_KEEP_PreservedCapabilities.md`, multi-level support guardrails

Implementation prompt:

1. Inspect current Showcase project and level discovery.
2. Identify the curated default level set.
3. Add the smallest catalog/manifest needed to describe levels and optional packs.
4. Teach launcher/cooker/runtime to select available levels through that metadata.
5. Mark heavyweight content as optional.
6. Move/delete heavy content only after default levels still run.

Target paths:

- `Projects/Showcase`
- `Projects/Showcase/Assets/Meshes/Bistro`
- large DDS/TGA/HDR assets
- launcher project selection
- cooker project/level discovery

Search patterns:

```powershell
rg -n "Showcase|Level|Scene|Bistro|\\.hdr|\\.tga|\\.dds|Cook" Projects Tools Engine
```

Allowed additions:

- minimal level/content catalog
- optional pack id/path metadata
- resolver logic only if it enables content removal

Forbidden additions:

- asset database rewrite
- content browser rewrite
- package manager
- network downloader unless already product-owned

Done criteria:

- [ ] Multiple levels remain selectable.
- [ ] Default level set cooks and launches without optional packs.
- [ ] Missing optional packs fail gracefully.
- [ ] Heavy content can be removed from the core repo.
- [ ] Depot bytes decrease materially.

## Phase Note 3: Screenshot/BMP Capture Hardening

Goal:

- preserve screenshot/BMP capture
- remove smoke/ad hoc ownership
- narrow API cost

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 2
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 3
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 2
- `01_KEEP_PreservedCapabilities.md`, capture guardrails

Implementation prompt:

1. Find all capture call sites.
2. Identify which are smoke/ad hoc paths.
3. Preserve or create one product-owned editor/tool capture entrypoint.
4. Keep RHI/BMP writer implementation only behind the hardened path.
5. Remove smoke-owned capture.

Target paths:

- `Engine/RHI/Public/Capture/RhiCaptureService.h`
- `Engine/RHI/Private/Capture/RhiBmpWriter.*`
- smoke/ad hoc capture call sites
- editor/tool capture command paths

Search patterns:

```powershell
rg -n "CaptureTextureToBmp|RhiCaptureService|RhiBmpWriter|Screenshot|BMP|Smoke" Engine Tools
```

Done criteria:

- [ ] Capture still writes BMP through intended path.
- [ ] Smoke/ad hoc capture ownership is gone.
- [ ] Public capture surface is smaller or clearly product-owned.
- [ ] D3D12/Vulkan readback behavior remains intact where supported.

## Phase Note 4: Smoke, Validation, Cook Report, And Debug Artifact Cleanup

Goal:

- remove default-path validation/report/debug artifact behavior
- preserve fatal guardrails and real product outputs

Use:

- `04_REMOVE_DeletionsAndCleanup.md`, Remove Categories 2 and 3
- `02_MODIFY_RefactorExistingSystems.md`, sections 2 and 7

Implementation prompt:

1. Remove application-level smoke validation if still present.
2. Remove launcher smoke orchestration if still present.
3. Remove renderer smoke snapshots.
4. Remove AssetCooker plan/timing/summary artifacts from default cook.
5. Remove shader debug artifacts from default launcher/cook.
6. Keep fatal errors and product outputs.

Target paths:

- `Engine/Application/Private/Validation/RhiSmoke*`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*`
- `Engine/Renderer/Public/Diagnostics/*Smoke*`
- `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.*`
- shader debug artifact writers

Search patterns:

```powershell
rg -n "RhiSmoke|SmokeDiagnostics|SPARKLE_SMOKE|asset-cooker-plan-v1|asset-cooker-summary-v1|debug-artifacts|timing summary|dry-run" Engine Tools
```

Done criteria:

- [ ] Default cook writes assets, not reports.
- [ ] Shader cook writes runtime packages, not debug bundles.
- [ ] Runtime/editor launch still works.
- [ ] PIX/RenderDoc/Nsight markers remain.
- [ ] Deleted files/lines greatly exceed added lines.

## Phase Note 5: Launcher Workflow Shell

Goal:

- reduce launcher to product workflows
- stop it being a diagnostic cockpit

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 3
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 7

Implementation prompt:

1. Identify launcher workflows actually owned: build, cook, run, clean, package if real.
2. Remove diagnostic-only pages/actions.
3. Remove status pages not tied to product workflow.
4. Remove shader debug/stat toggles from default GUI.
5. Remove package UI for unowned packages.
6. Keep command paths for real workflows.

Target paths:

- `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/*`
- option pages
- operation/status pages
- package workflow files
- shader/debug option code

Search patterns:

```powershell
rg -n "Status|Diagnostic|DebugArtifact|ShaderStats|Package|Operation|DryRun|Quality|Smoke" Tools/Launcher/SparkleLauncher
```

Done criteria:

- [ ] Launcher source decreases.
- [ ] Build/cook/run/clean still work.
- [ ] Package remains only if product-owned.
- [ ] No new panel replaces removed panels.

## Phase Note 6: Public Renderer/RHI Observation API Narrowing

Goal:

- shrink public APIs around behavior instead of observation
- keep compact facts only if they drive runtime policy

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 4
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 4
- `01_KEEP_PreservedCapabilities.md`, RHI/Renderer guardrails

Implementation prompt:

1. List public diagnostic/observation headers.
2. Identify real consumers.
3. Move editor-only observation private or delete unowned panels.
4. Collapse memory/descriptor snapshots into compact status only if consumed by policy.
5. Preserve debugger markers, timestamps, object names, fatal diagnostics, and capture.

Target paths:

- `Engine/Renderer/Public/Renderer.h`
- `Engine/Renderer/Public/Diagnostics/*`
- `Engine/Renderer/Public/Meshes/MeshDiagnostics.h`
- `Engine/Renderer/Public/Resources/Textures/TextureDiagnostics.h`
- `Engine/RHI/Public/Diagnostics/*`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`
- editor panels consuming diagnostics

Search patterns:

```powershell
rg -n "RendererMemoryDiagnostics|MeshDiagnostics|TextureDiagnostics|RhiDiagnostics|DescriptorUsage|Snapshot|Dump|Json|Panel" Engine
```

Done criteria:

- [ ] Public renderer/RHI header count or line count decreases.
- [ ] Editor still has only product-owned panels.
- [ ] No new diagnostics facade appears.
- [ ] Runtime pressure facts remain if used by policy.

## Phase Note 7: Classic TLAS And PTLAS Refactor

Goal:

- preserve both classic TLAS and PTLAS
- minimize PTLAS toward original reference-style flow
- preserve D3D12 and Vulkan support where available

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 5
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 2
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 4 if capability facts are needed
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 5
- `01_KEEP_PreservedCapabilities.md`, ray tracing guardrails

Implementation prompt:

1. Trace current classic TLAS and PTLAS build/update/trace flows.
2. Identify strategy selection points.
3. Preserve selection between classic TLAS and PTLAS.
4. Remove future GPU-pack placeholders.
5. Remove PTLAS planner metrics/diagnostics not needed for build/update/trace.
6. Remove CPU validation readbacks not required for product behavior.
7. Collapse PTLAS toward capability check, descriptor input, backend call, lifetime, trace use.

Target paths:

- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.*`
- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlas*`
- `Engine/Renderer/Private/RayTracing/Diagnostics/RayTracingPtlas*`
- `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h`
- D3D12 PTLAS services
- Vulkan PTLAS services

Search patterns:

```powershell
rg -n "PTLAS|Ptlas|PartitionedTlas|future GPU pack|placeholder|AllowCpuValidationReadback|TLAS|BLAS" Engine
```

Done criteria:

- [ ] Classic TLAS renders.
- [ ] PTLAS renders where supported.
- [ ] User selection can choose classic TLAS or PTLAS.
- [ ] D3D12 and Vulkan capability checks remain honest.
- [ ] PTLAS code is smaller and more direct.
- [ ] No future GPU-pack scaffolding remains in default path.

## Phase Note 8: Reference Path Tracing Role Cleanup

Goal:

- make reference path tracing honest and scoped
- avoid a second ambiguous realtime renderer

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 6
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 8
- `G_AdvancedGraphicsEngineExecutiveSummary.md`, Ray Tracing/GI/Path Tracing section

Implementation prompt:

1. Choose debug reference as the first role.
2. Remove unsupported provider handoff hooks.
3. Remove guide outputs not consumed by product features.
4. Remove settings that imply unsupported realtime product quality.
5. Share BRDF/material/light policy with realtime path.

Target areas:

- reference frame path
- reference passes
- reference shaders
- provider handoff code
- reference settings/CVars

Search patterns:

```powershell
rg -n "Reference|PathTrace|PathTracing|Guide|RayReconstruction|MotionVector|Accumulation|Progressive" Engine/Renderer Engine/Assets/Shaders
```

Done criteria:

- [ ] Reference mode has one sentence role.
- [ ] Dead provider handoff is gone.
- [ ] Reference buffers/settings are fewer or explicitly consumed.
- [ ] Material/light policy aligns with realtime path.

## Phase Note 9: Shader Debug And Duplication Cleanup

Goal:

- preserve shader compiler/cook/runtime ABI
- remove debug/demo bloat from default workflows

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 7
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 6
- `01_KEEP_PreservedCapabilities.md`, Shader Pipeline section

Implementation prompt:

1. Identify default shader debug artifacts.
2. Remove debug artifact generation from default cook.
3. Remove cooked stats CSV from default workflow unless actively consumed.
4. Remove validation-only shader registrations from product runtime.
5. Preserve reflection, layout safety, package cooking, runtime package cache.

Target paths:

- `Tools/Shaders/ShaderCompiler`
- `Tools/Shaders/ShaderContracts`
- `Engine/Renderer/ShaderRegistrations`
- launcher shader options
- shader debug artifact writers

Search patterns:

```powershell
rg -n "debug-artifacts|ShaderStats|stats CSV|HelloWorld|ComputeClear|artifact|reflection|package" Engine Tools
```

Done criteria:

- [ ] Default shader cook emits runtime packages only.
- [ ] Debug bundles are opt-in or removed.
- [ ] Shader packages still load at runtime.
- [ ] Renderer shaders still compile.

## Phase Note 10: Core And GameFramework Public Surface Cleanup

Goal:

- reduce public engine surface
- keep stable runtime contracts

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 8
- `D_WholeRepositoryArchitectureMap.md`, Public Versus Private API Shape

Implementation prompt:

1. Identify public Core and GameFramework headers that are implementation details.
2. Move private-only helpers private.
3. Remove unused utility APIs.
4. Narrow asset loader and scene manifest exposure.
5. Keep level/scene/component/asset types projects actually use.

Target paths:

- `Engine/Core/Public`
- `Engine/GameFramework/Public`
- `Engine/GameFramework/Private`
- cooked scene loaders
- asset loader/scene manifest types

Search patterns:

```powershell
rg -n "Manifest|Loader|Scene|Level|Component|Asset|Utils|Helper" Engine/Core Engine/GameFramework
```

Done criteria:

- [ ] Public Core/GameFramework lines decrease.
- [ ] Cooked scene still loads.
- [ ] No replacement aggregate header appears.
- [ ] Renderer still consumes GameFramework privately.

## Phase Note 11: Renderer Feature Hardening Before Measurement

Goal:

- make the feature surface worth profiling
- harden renderer features without adding measurement systems

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 9
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 5
- `H_AdvancedGraphicsEngineerPersona.md`, Renderer Feature Depth and Shader/Kernel Craft

Implementation prompt:

1. Pick one feature area: RT, GI/path tracing, post-processing, denoising, upscaling, frame graph, shaders, or passes.
2. Identify old ambiguity or duplicate ownership.
3. Implement only a vertical slice that replaces or simplifies old code.
4. Keep pass inputs/outputs obvious.
5. Preserve existing debugger/profiler hooks.
6. Do not add benchmarking/reporting systems.

Target areas:

- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/Frame`
- `Engine/Renderer/Private/Upscaling`
- `Engine/Renderer/Private/RayReconstruction`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Assets/Shaders/Passes`

Search patterns:

```powershell
rg -n "Denois|Upscal|RayReconstruction|Post|Composite|FrameGraph|Pass|Resource|History|Provider|Fallback" Engine/Renderer Engine/Assets/Shaders
```

Done criteria:

- [ ] Feature-facing renderer/RHI code is smaller or simpler.
- [ ] No new docs/logs/validation/report systems appear.
- [ ] Affected feature remains buildable.
- [ ] Existing markers/timestamps/capture remain.

## Phase Note 12: Package Contract Cleanup

Goal:

- keep only real package outputs
- separate optional content from runtime package

Use:

- `02_MODIFY_RefactorExistingSystems.md`, section 10
- `03_ADD_MinimalMissingCapabilities.md`, Allowed Addition 6
- `04_REMOVE_DeletionsAndCleanup.md`, Remove Category 9

Implementation prompt:

1. Decide which packages are real now: runtime, editor, launcher, dev tools, symbols, optional content.
2. Remove package assembly code for unowned outputs.
3. Remove future manifest fields not consumed.
4. Remove release note generation not consumed.
5. Keep checksums/manifests only if workflow consumes them.
6. Keep optional content out of runtime package.

Target paths:

- `CMake/SparkleReleaseAssembly.cmake`
- launcher package workflow files
- artifact contract CMake files

Search patterns:

```powershell
rg -n "Package|Manifest|Release|Checksum|Artifact|Symbols|ContentPack|Assembly" CMake Tools
```

Done criteria:

- [ ] One package command produces only owned outputs.
- [ ] Package/build code decreases or package output bytes decrease.
- [ ] Optional content is separate from runtime package.

## Phase Note 13: Late Profiling And Measurement

Goal:

- profile only after feature cleanup
- use professional tools and existing hooks first

Use:

- `03_ADD_MinimalMissingCapabilities.md`, Late Additions Only
- `G_AdvancedGraphicsEngineExecutiveSummary.md`, Late CPU/GPU Performance Evidence
- `D_WholeRepositoryArchitectureMap.md`, CPU/GPU Performance Maps

Implementation prompt:

1. Verify stages 2 through 12 are substantially complete for the target area.
2. Use PIX/RenderDoc/Nsight and existing markers first.
3. Use existing timestamps before adding measurement systems.
4. Add measurement code only if it replaces multiple old diagnostics.
5. Keep net code non-positive unless explicitly approved.

Measurement targets:

- CPU frame setup/compile
- scene snapshot/build cost
- texture loading
- shader package load
- renderer submit
- GPU pass timings
- ray tracing build/update timings
- memory budget/transient pressure
- descriptor allocator occupancy
- pipeline/shader package count

Done criteria:

- [ ] Before/after evidence exists.
- [ ] No new log/report panel appears.
- [ ] Existing markers/timestamps remain.
- [ ] Measurement code replaces old diagnostics or remains very small.

## Phase Note 14: Final Readiness Pass

Goal:

- confirm the full scope is covered
- confirm persona and engine direction are real in the repo

Use:

- all four implementation docs
- `H_AdvancedGraphicsEngineerPersona.md`
- `G_AdvancedGraphicsEngineExecutiveSummary.md`
- `D_WholeRepositoryArchitectureMap.md`

Final checks:

- [ ] Core repo size is much smaller.
- [ ] Multi-level project support remains intact.
- [ ] Public Renderer/RHI/Core/GameFramework API is smaller.
- [ ] Launcher is workflow-focused.
- [ ] Default cook writes assets, not diagnostics.
- [ ] Default shader cook writes runtime packages, not debug bundles.
- [ ] Classic TLAS and PTLAS are both product-owned and selectable where supported.
- [ ] PTLAS is smaller and closer to reference-style flow.
- [ ] Reference path has a clear role.
- [ ] Frame graph remains the one render scheduling abstraction.
- [ ] RHI remains explicit.
- [ ] D3D12 and Vulkan remain first-class, with parity preserved or extended.
- [ ] Screenshot/BMP capture remains preserved and hardened.
- [ ] PIX/RenderDoc/Nsight support remains strong.
- [ ] Heavy optional content is out of the default footprint.
- [ ] No new docs/logs/validation/report systems/wrappers/scaffolding have replaced old ones.
- [ ] The next feature can delete or replace something old.

Final stabilization checks:

- [ ] Build the relevant editor/runtime targets.
- [ ] Cook the curated default level set.
- [ ] Run the default level set.
- [ ] Run both D3D12 and Vulkan paths where the local machine supports them.
- [ ] Verify shader packages cook and load with reflection data.
- [ ] Verify screenshot/BMP capture through the hardened path.
- [ ] Verify classic TLAS and PTLAS selection where supported.
- [ ] Verify multiple levels remain selectable.

## Weekly Working Pattern

A sustainable cadence:

1. Monday: choose one roadmap stage and fill the batch prompt.
2. Tuesday-Wednesday: inspect code and implement the smallest meaningful batch.
3. Thursday: run targeted checks for the edited subsystem and record any final stabilization risk.
4. Friday: remove leftover dead code, update only stale existing docs, and record the next batch.

Keep each batch small enough to review. The plan can take weeks; that is fine. The important thing is that every completed batch leaves Sparkle smaller, clearer, and closer to the advanced graphics engineer persona.

## What To Open First

For a normal implementation session, open in this order:

1. `00_ORDERED_ImplementationRoadmap.md`
2. `01_KEEP_PreservedCapabilities.md`
3. the exact section in `02_MODIFY_RefactorExistingSystems.md`, `03_ADD_MinimalMissingCapabilities.md`, or `04_REMOVE_DeletionsAndCleanup.md`
4. the source review doc only if you need background:
   - current state: `D_WholeRepositoryArchitectureMap.md`
   - external comparison: `E_ExternalRendererRepositoryComparison.md`
   - persona: `H_AdvancedGraphicsEngineerPersona.md`
   - executive priority: `G_AdvancedGraphicsEngineExecutiveSummary.md`

Do not open every doc every time. Use this roadmap as the index, then jump to the exact implementation prompt.
