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
6. Verify the acceptance criteria from the stage with targeted checks; defer full build/cook/run stabilization to Stage 14 unless you intentionally choose a checkpoint.
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
Primary source docs:
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

## Stage Overview

| Stage | Theme | Primary doc | Result |
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

## Stage 0: Baseline And Guardrails

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
4. Save the final build/cook/run commands you will use in Stage 14.

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
- [ ] You know the final build/cook/run commands for Stage 14.
- [ ] You know which KEEP items the batch must preserve.
- [ ] You have identified whether the batch is MODIFY, ADD, REMOVE, or mixed.

## Stage 1: Repository And Documentation Hygiene

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

## Stage 2: Content Catalog And Optional Heavy Packs

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

## Stage 3: Screenshot/BMP Capture Hardening

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

## Stage 4: Smoke, Validation, Cook Report, And Debug Artifact Cleanup

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

## Stage 5: Launcher Workflow Shell

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

## Stage 6: Public Renderer/RHI Observation API Narrowing

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

## Stage 7: Classic TLAS And PTLAS Refactor

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

## Stage 8: Reference Path Tracing Role Cleanup

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

## Stage 9: Shader Debug And Duplication Cleanup

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

## Stage 10: Core And GameFramework Public Surface Cleanup

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

## Stage 11: Renderer Feature Hardening Before Measurement

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

## Stage 12: Package Contract Cleanup

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

## Stage 13: Late Profiling And Measurement

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

## Stage 14: Final Readiness Pass

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
