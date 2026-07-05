# 04. REMOVE - Deletions And Cleanup

Status: deletion-first implementation prompt
Date: 2026-07-04
Source set: `Docs/Architecture/00-Review`
Use this as: the deletion queue and cleanup checklist

## Purpose

This document gathers what should be removed, collapsed, externalized, or moved out of default workflows.

The goal is not to make Sparkle smaller by deleting capability. The goal is to delete everything that makes real capability harder to see:

- smoke-only code
- validation-only systems
- report-only code
- debug artifact writers
- future scaffolding
- wrapper-only layers
- thick abstraction layers that make rendering behavior harder to trace
- public observation APIs
- uncataloged heavy content
- diagnostic launcher/cooker behavior

Preserve:

- D3D12/Vulkan
- proper Core/RHI/Renderer/GameFramework/Tools/Projects separation
- RHI explicitness
- frame graph
- offline cooked shader packages with reflection data
- classic TLAS and PTLAS
- screenshot/BMP capture
- PIX/RenderDoc/Nsight support
- multi-level project support
- final build/cook/run workflows

## Removal Gate

Before deleting, answer:

1. Is the target a capability, a thin user-facing concept, or scaffolding around a capability?
2. If it is a capability, move to `01_KEEP_PreservedCapabilities.md`.
3. If it is a thin high-level concept that hides low-level details and makes the engine easier to reason about, preserve it or move it to `02_MODIFY_RefactorExistingSystems.md`.
4. If it is valuable but bloated, move to `02_MODIFY_RefactorExistingSystems.md` and remove bloat as the top priority.
5. If it is thick abstraction, duplicate indirection, default diagnostic/report behavior, or future scaffolding, keep it in this removal document.
6. If deletion requires a small replacement to preserve behavior, move to `03_ADD_MinimalMissingCapabilities.md`.
7. Does the deletion preserve or improve D3D12/Vulkan parity?
8. Does it preserve offline cooked shader packages with reflection data?
9. Does it preserve screenshot capture?
10. Does it preserve classic TLAS and PTLAS?
11. Does it preserve multi-level support?
12. Does it preserve Core/RHI/Renderer/GameFramework/Tools/Projects separation?
13. Is any build/cook/run risk recorded for the final stabilization pass?
14. If a replacement is proposed, does existing code already provide it?
15. Does the deletion remove duplicate responsibility rather than adding a second owner?
16. Does the net batch remove or simplify at least as much code as it adds?
17. Does real code remain free of hardcoded project, level, asset, optional-pack, and sample names?
18. Does the change avoid fallback chains and fail clearly for missing required data?
19. Did deletion leave a wrapper function that only forwards to another owner?
20. Did deletion leave a struct/class/config object with one meaningful field and no behavior, invariant, or external ABI role?
21. Did deletion leave empty `if`, `else`, loop, switch, callback, or no-op function bodies?
22. Did deletion leave duplicated constants, marker names, path normalization helpers, capability discovery, or local utility code outside the owning module?
23. Were call sites, includes, source lists, and docs propagated to the surviving owner?

Only delete when the answer is clear.

## First Cut Order

Recommended first deletion batches:

1. Catalog levels and externalize heavy Showcase media.
2. Delete smoke/ad hoc ownership of screenshot capture while preserving capture.
3. Remove AssetCooker plan/timing/summary artifacts from default cook.
4. Remove launcher shader debug artifact/stat toggles from default GUI.
5. Delete PTLAS future GPU-pack placeholders while preserving PTLAS.
6. Move renderer mesh/texture/memory diagnostics out of public API or delete unowned panels.
7. Cut launcher operation/status UI until it is build, cook, run, clean, package if owned.

## Planned Deletion Capability Audit

Use this table before starting any deletion batch. The intent is to delete scaffolding around capability, never the capability itself.

| Deletion Target | Capability Protected | Owner To Check | Safe Result |
| --- | --- | --- | --- |
| Heavy uncataloged content | Multi-level support | Projects, GameFramework, Tools | Heavy content leaves the default footprint only after levels remain discoverable, selectable, cookable, and runnable. |
| Smoke harnesses | Screenshot/BMP capture, shader cook, asset cook, runtime launch, debugger markers | RHI, Renderer, Application, Tools | Smoke/ad hoc ownership is removed; product launch, cook, capture, and fatal checks remain. |
| Default cook reports and diagnostic artifacts | Offline cooked assets and shader packages with reflection data | Tools, RHI, Renderer | Default workflows emit product assets/packages and fatal errors, not reports. |
| Public observation APIs | RHI/Renderer behavior and compact runtime pressure facts | RHI, Renderer, Editor | Public diagnostics shrink; debugger markers, object names, timestamps, and consumed pressure facts remain. |
| PTLAS scaffolding | Classic TLAS and PTLAS | Renderer, RHI | PTLAS becomes smaller and closer to reference flow while classic TLAS, PTLAS, backend support, and selection remain. |
| Shader debug/demo bloat | Offline cooked shader packages with reflection data | Tools, RHI, Renderer | Debug/demo artifacts leave default paths; ABI, reflection, package cache, and registrations remain coherent. |
| Launcher diagnostic cockpit behavior | Build/cook/run/clean/package-if-owned workflows | Tools | Launcher becomes a workflow shell, not a diagnostic product. |
| Reference path ambiguity | Ray tracing, GI, path tracing evidence | Renderer, RHI | Reference path keeps one clear role and shared material/light policy. |
| Package/release scaffolding | Product-owned package outputs and optional content separation | Tools, Projects | Only owned packages remain; optional content is separate from runtime output. |
| Stale docs/review noise | Active implementation guidance | Docs | Navigation gets clearer without adding another planning layer. |

## Remove Category 1: Heavy Uncataloged Content

Why remove:

- `Projects` is about 1527.06 MB.
- Content weight dominates repo cost.
- Heavy content should not be required for every clone/build/review.

Remove or externalize:

- `Projects/Showcase/Assets/Meshes/Bistro`
- large DDS/TGA texture sets
- large HDR skyboxes not needed by default level set
- duplicated media
- generated content that can be rebuilt

Preserve:

- multiple levels
- curated default level set
- project launch/cook/run behavior
- optional content discoverability

Search prompts:

```powershell
rg -n "Bistro|\\.hdr|\\.tga|\\.dds" Projects/Showcase
```

Acceptance:

- [x] Heavy content is represented by optional pack metadata before removal.
- [x] Default level set runs without optional packs.
- [x] Repo byte size drops materially.
- [x] Multi-level support remains intact.

Stage 06 removal boundary:

- `Projects/Showcase/Assets/Meshes/Bistro` is cataloged as optional pack `Bistro` with root `Assets/Meshes/Bistro`.
- Stage 07 removed it from the core repo.
- The removed/externalized payload was about 1438.80 MB.

## Remove Category 2: Smoke Harnesses

Why remove:

- Smoke harnesses can preserve validation/report scaffolding that no longer represents product behavior.

Remove:

- app-level RHI smoke validation
- launcher smoke orchestration
- renderer smoke snapshot APIs
- smoke-only capture paths
- smoke-specific environment/config flags

Preserve:

- fatal API/result checks
- normal editor/runtime launch
- screenshot capture through hardened product path
- shader cook
- asset cook
- debugger markers

Target paths and patterns:

```powershell
rg -n "RhiSmoke|SmokeDiagnostics|SPARKLE_SMOKE|Smoke" Engine Tools
```

Likely areas:

- `Engine/Application/Private/Validation/RhiSmoke*`
- `Tools/Launcher/SparkleLauncher/Private/Launch/Smoke/*`
- `Engine/Renderer/Public/Diagnostics/*Smoke*`

Acceptance:

- [ ] Smoke search returns only intentionally preserved product references.
- [ ] Runtime/editor launch still works.
- [ ] Capture still works through hardened path.
- [ ] Deleted files/lines greatly exceed added lines.

## Remove Category 3: Default Cook Reports And Diagnostic Artifacts

Why remove:

- Cookers should emit cooked assets and clear fatal errors, not durable diagnostic reports by default.

Remove from default workflows:

- asset-cooker plan artifacts
- asset-cooker timing summaries
- default diagnostic summaries
- dry-run report schemas if not product-owned
- debug artifact output from normal cook

Preserve:

- texture/mesh/material/scene cook outputs
- clear fatal errors
- shader package cook
- import paths needed by cookers

Target paths and patterns:

```powershell
rg -n "asset-cooker-plan-v1|asset-cooker-summary-v1|timing|summary|debug-artifacts" Tools Engine
```

Likely areas:

- `Tools/Cooking/AssetCooker/Private/Dispatch/AssetCookerDispatcher.cpp`
- `Tools/Cooking/AssetCooker/Private/Discovery/AssetCookerDiscovery.*`
- shader debug artifact writers
- launcher cook/debug toggles

Acceptance:

- [ ] Default cook writes assets, not reports.
- [ ] Fatal cook errors remain clear.
- [ ] Shader cook writes runtime packages.
- [ ] Report formats are gone or opt-in developer tools only.

## Remove Category 4: Public Observation APIs

Why remove:

- Public observation surfaces look like permanent product contracts.
- Many are only useful for editor/debug panels.

Remove or move private:

- public mesh diagnostics
- public texture diagnostics
- broad memory diagnostics snapshots
- descriptor dump APIs
- JSON/detail allocation lists
- public renderer smoke snapshots
- editor-only diagnostic APIs

Preserve:

- compact runtime pressure facts if consumed by policy
- fatal diagnostics
- RHI debug layers
- GPU events
- object names
- timestamps
- screenshot capture

Target paths:

- `Engine/Renderer/Public/Diagnostics/*`
- `Engine/Renderer/Public/Meshes/MeshDiagnostics.h`
- `Engine/Renderer/Public/Resources/Textures/TextureDiagnostics.h`
- `Engine/RHI/Public/Diagnostics/*`
- `Engine/RHI/Public/Descriptors/RhiDescriptorService.h`

Acceptance:

- [ ] Public header count or public lines decrease.
- [ ] Editor panels either become product-owned or are removed.
- [ ] No new diagnostics facade replaces deleted APIs.
- [ ] Runtime policy still has required pressure facts.

## Remove Category 5: PTLAS Scaffolding, Not PTLAS

Why remove:

- PTLAS is a preserved product capability.
- Extra planner, metrics, CPU validation, and future GPU-pack code makes it look experimental and expensive.

Remove:

- future GPU-pack placeholders
- placeholder passes/resources
- PTLAS planner metrics not needed to build/update/trace
- diagnostic structs not consumed by product behavior
- CPU validation readbacks not required for product path
- duplicate selection/fallback paths

Preserve:

- PTLAS descriptor inputs required by backend
- D3D12 PTLAS service
- Vulkan PTLAS service where supported
- classic TLAS
- user-facing TLAS/PTLAS selection

Search prompts:

```powershell
rg -n "future GPU pack|placeholder|AllowCpuValidationReadback|Ptlas|PTLAS|PartitionedTlas" Engine
```

Target paths:

- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.*`
- `Engine/Renderer/Private/RayTracing/Acceleration/RayTracingPtlas*`
- `Engine/Renderer/Private/RayTracing/Diagnostics/RayTracingPtlas*`
- `Engine/RHI/Public/RayTracing/RhiPartitionedTlasDesc.h`
- D3D12/Vulkan PTLAS services

Acceptance:

- [ ] Classic TLAS still renders.
- [ ] PTLAS still renders where supported.
- [ ] PTLAS code is smaller and closer to reference flow.
- [ ] Future GPU-pack search returns no default-path scaffolding.

## Remove Category 6: Shader Debug/Demo Bloat

Why remove:

- Shader compiler/cook/runtime ABI is a strength.
- Default debug bundles and demo registrations make the shader pipeline look like a validation lab.

Remove from default runtime/workflow:

- debug artifact bundles
- cooked shader stats CSV by default
- launcher debug artifact toggles
- validation-only shader registrations
- sample shader demos that do not serve renderer features

Preserve:

- HLSL/Slang compiler path
- shader reflection
- package ABI
- layout safety
- runtime package cache
- package inspection if useful

Search prompts:

```powershell
rg -n "debug-artifacts|stats CSV|HelloWorld|ComputeClear|ShaderStats|artifact" Engine Tools
```

Acceptance:

- [ ] Default shader cook emits runtime packages only.
- [ ] Debug bundles are opt-in or removed.
- [ ] Renderer shaders still compile.
- [ ] Runtime package cache still loads packages.

## Remove Category 7: Launcher Diagnostic Cockpit Behavior

Why remove:

- Launcher should be workflow shell, not a diagnostic/status/report application.

Remove:

- diagnostic-only pages/actions
- status pages not tied to product workflow
- quality checks not actively owned
- dry-run plans if not consumed
- package UI for unowned packages
- shader debug toggles from default GUI

Preserve:

- build
- cook
- run
- clean
- package if owned
- dependency sync if actively used

Target paths:

- `Tools/Launcher/SparkleLauncher/Private/Shell/LauncherShell.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/*`
- launcher option pages
- launcher package workflow files
- launcher shader/debug option code

Acceptance:

- [ ] Launcher source decreases.
- [ ] Build/cook/run/clean still work.
- [ ] Package remains only if product-owned.
- [ ] No new panels replace removed panels.

## Remove Category 8: Reference Path Ambiguity

Why remove:

- Reference path tracing needs one role first.
- Ambiguous buffers/settings create feature sprawl.

Remove:

- provider handoff hooks that are unsupported
- guide outputs not consumed by product features
- settings that imply a second realtime renderer without product quality
- reference-only buffers with no clear use

Preserve:

- debug reference path
- material/light correctness comparison
- required shader code

Acceptance:

- [ ] Reference mode has one product role.
- [ ] Dead provider handoff is gone.
- [ ] Fewer reference-specific buffers/settings remain.

## Remove Category 9: Package And Release Scaffolding

Why remove:

- Package code should match real outputs.

Remove:

- unowned package assembly code
- future manifest fields
- release note generation not consumed
- checksums not consumed
- content bundled into runtime package when it should be optional

Preserve:

- runtime package if product-owned
- editor package if product-owned
- launcher package if product-owned
- dev tools package if product-owned
- optional content package if product-owned

Target paths:

- `CMake/SparkleReleaseAssembly.cmake`
- launcher package workflow files
- artifact contract CMake files

Acceptance:

- [ ] One package command produces only owned outputs.
- [ ] Package/build code decreases or output bytes decrease.
- [ ] Optional content is separate.

## Remove Category 10: Stale Docs And Review Noise

Why remove:

- The new implementation spine is four docs.
- Old docs should be source material, not the daily execution path.

Remove or update:

- stale README entries
- direct source-trail notes in background docs
- references to deleted review docs
- duplicated review plans
- planning text that asks for new docs/validation instead of cleanup

Preserve:

- `00-Review` as source material
- `01-Implementation` as active execution spine
- neutral wording

Acceptance:

- [ ] README points to implementation spine first.
- [ ] No stale doc links remain.
- [ ] No sensitive source trail remains.
- [ ] Existing docs support implementation rather than scatter it.

## Batch Template

Use this exact structure when executing a removal batch:

```text
Batch name:
Persona pillar:
Capability preserved:
Files to remove/modify:
Search patterns used:
Pre-delete behavior to preserve:
Deletion target:
Fallback if capability breaks:
Cleanup-after-cleanup scan:
Final build/cook/run stabilization risk:
Targeted verification before final stage:
Expected net delta:
```

## Done State

The remove plan is successful when:

- repo byte size is much smaller
- public API is smaller
- launcher is smaller
- cookers emit assets, not reports
- shader cook emits packages, not debug bundles
- PTLAS is smaller but preserved
- screenshot capture is preserved
- multiple levels remain supported
- no new replacement scaffolding appears
- no no-value forwarding wrappers, single-field data-only shells, empty control-flow shells, stale includes, duplicated local helpers, or dead compatibility paths remain in touched modules
